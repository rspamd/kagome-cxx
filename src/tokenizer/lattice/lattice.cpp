#include "kagome/tokenizer/lattice/lattice.hpp"
#include <unicode/utf8.h>
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <fmt/format.h>
#include <limits>
#include <algorithm>

namespace kagome::tokenizer::lattice {

// Constants for search mode penalties
constexpr std::int32_t MAXIMUM_COST = std::numeric_limits<std::int32_t>::max();
constexpr std::int32_t MAXIMUM_UNKNOWN_WORD_LENGTH = 1024;
constexpr std::int32_t SEARCH_MODE_KANJI_LENGTH = 2;
constexpr std::int32_t SEARCH_MODE_KANJI_PENALTY = 3000;
constexpr std::int32_t SEARCH_MODE_OTHER_LENGTH = 7;
constexpr std::int32_t SEARCH_MODE_OTHER_PENALTY = 1700;

// Static memory pool
ObjectPool<Node> Lattice::node_pool_;

// Helper function to count Unicode characters in UTF-8 string
static std::int32_t count_utf8_chars(std::string_view str)
{
	std::int32_t char_count = 0;
	std::int32_t byte_pos = 0;
	std::int32_t str_len = static_cast<std::int32_t>(str.length());

	while (byte_pos < str_len) {
		UChar32 codepoint;
		U8_NEXT(reinterpret_cast<const uint8_t *>(str.data()), byte_pos, str_len, codepoint);
		if (codepoint >= 0) {
			char_count++;
		}
	}

	return char_count;
}

Lattice::Lattice(std::shared_ptr<dict::Dict> dictionary,
				 std::shared_ptr<dict::UserDict> user_dictionary)
	: dict_(std::move(dictionary)), user_dict_(std::move(user_dictionary))
{
}

void Lattice::build(std::string_view input)
{
	input_ = std::string(input);

	// Clear previous state
	clear();

	// Count Unicode characters for proper sizing
	std::int32_t char_count = count_utf8_chars(input);

	// Resize node list
	node_list_.resize(char_count + 2);

	// Add BOS and EOS nodes
	add_node(0, BOS_EOS_ID, 0, 0, NodeClass::Dummy, "");
	add_node(char_count + 1, BOS_EOS_ID, static_cast<std::int32_t>(input.length()),
			 char_count, NodeClass::Dummy, "");

	// Process each character position
	std::int32_t byte_pos = 0;
	std::int32_t char_pos = 0;

	while (byte_pos < static_cast<std::int32_t>(input.length())) {
		UChar32 current_char;

		// Extract Unicode character
		std::int32_t char_start_byte = byte_pos;
		U8_NEXT(reinterpret_cast<const uint8_t *>(input.data()), byte_pos,
				static_cast<std::int32_t>(input.length()), current_char);

		if (current_char < 0) {
			// Invalid UTF-8, skip
			continue;
		}

		bool any_matches = false;
		std::int32_t longest_match_bytes = 0;
		std::int32_t longest_match_chars = 0;

		// 1. Try user dictionary first
		if (user_dict_) {
			std::string_view remaining_input(input.data() + char_start_byte,
											 input.length() - char_start_byte);

			user_dict_->index.common_prefix_search_callback(
				remaining_input,
				[this, char_pos, char_start_byte, &any_matches, &longest_match_bytes, &longest_match_chars](std::int32_t id, std::int32_t length) {
					std::string surface(input_.substr(char_start_byte, length));
					add_node(char_pos, id, char_start_byte, char_pos,
							 NodeClass::User, std::move(surface));
					any_matches = true;

					// Track longest match
					if (length > longest_match_bytes) {
						longest_match_bytes = length;
						// Count characters in this match
						std::string_view match_surface(input_.data() + char_start_byte, length);
						longest_match_chars = count_utf8_chars(match_surface);
					}
				});
		}

		if (any_matches) {
			// Advance by the longest match found
			char_pos += longest_match_chars;
			byte_pos = char_start_byte + longest_match_bytes;
			continue;
		}

		// 2. Try system dictionary
		std::string_view remaining_input(input.data() + char_start_byte,
										 input.length() - char_start_byte);

		dict_->index.common_prefix_search_callback(
			std::string(remaining_input),
			[this, char_pos, char_start_byte, &any_matches, &longest_match_bytes, &longest_match_chars](std::int32_t id, std::int32_t length) {
				std::string surface(input_.substr(char_start_byte, length));
				add_node(char_pos, id, char_start_byte, char_pos,
						 NodeClass::Known, std::move(surface));
				any_matches = true;

				// Track longest match
				if (length > longest_match_bytes) {
					longest_match_bytes = length;
					// Count characters in this match
					std::string_view match_surface(input_.data() + char_start_byte, length);
					longest_match_chars = count_utf8_chars(match_surface);
				}
			});

		// If we found dictionary matches, advance and continue
		if (any_matches) {
			// Advance by the longest match found
			char_pos += longest_match_chars;
			byte_pos = char_start_byte + longest_match_bytes;
			continue;
		}

		// 3. Handle unknown words (only if no dictionary matches found)
		dict::CharacterCategory char_category = dict_->character_category(current_char);

		std::int32_t end_byte = byte_pos;
		std::int32_t unknown_word_len = 1;

		// Group consecutive characters of same category if needed
		if (dict_->should_group(char_category)) {
			while (byte_pos < static_cast<std::int32_t>(input.length()) &&
				   unknown_word_len < MAXIMUM_UNKNOWN_WORD_LENGTH) {
				UChar32 next_char;
				std::int32_t next_byte_pos = byte_pos;
				U8_NEXT(reinterpret_cast<const uint8_t *>(input.data()), next_byte_pos,
						static_cast<std::int32_t>(input.length()), next_char);

				if (next_char < 0 || dict_->character_category(next_char) != char_category) {
					break;
				}

				byte_pos = next_byte_pos;
				end_byte = byte_pos;
				unknown_word_len++;
			}
		}

		// Add unknown word entries
		if (static_cast<std::size_t>(char_category) < dict_->unk_dict.index.size()) {
			std::int32_t base_id = dict_->unk_dict.index[static_cast<std::size_t>(char_category)];
			std::int32_t dup_count = 1;

			if (static_cast<std::size_t>(char_category) < dict_->unk_dict.index_dup.size()) {
				dup_count = dict_->unk_dict.index_dup[static_cast<std::size_t>(char_category)] + 1;
			}

			for (std::int32_t i = 0; i < dup_count; ++i) {
				// Add both full word and truncated version if multi-character
				if (unknown_word_len > 1) {
					// Add truncated version (all but last character)
					std::int32_t truncated_end = end_byte;
					// Move back one character
					std::int32_t temp_pos = end_byte;
					U8_BACK_1(reinterpret_cast<const uint8_t *>(input.data()), 0, temp_pos);
					truncated_end = temp_pos;

					std::string truncated_surface = input_.substr(char_start_byte, truncated_end - char_start_byte);
					add_node(char_pos, base_id + i, char_start_byte, char_pos,
							 NodeClass::Unknown, truncated_surface);
				}

				// Add full word
				std::string full_surface = input_.substr(char_start_byte, end_byte - char_start_byte);
				add_node(char_pos, base_id + i, char_start_byte, char_pos,
						 NodeClass::Unknown, std::move(full_surface));
			}
		}
		else {
			// Character category not in unk_dict - create basic unknown node to maintain lattice connectivity
			// This is critical for mixed content (ASCII + Japanese) to work properly
			std::string full_surface = input_.substr(char_start_byte, end_byte - char_start_byte);
			add_node(char_pos, -2, char_start_byte, char_pos,// Use special ID for unmapped categories
					 NodeClass::Unknown, std::move(full_surface));
		}

		// Advance by the number of characters consumed by unknown word
		char_pos += unknown_word_len;
		// byte_pos is already advanced by the unknown word processing
	}
}

void Lattice::add_node(std::int32_t pos, std::int32_t id, std::int32_t position,
					   std::int32_t start, NodeClass node_class, std::string surface)
{
	dict::Morph morph;

	switch (node_class) {
	case NodeClass::Known:
		if (static_cast<std::size_t>(id) < dict_->morphs.size()) {
			morph = dict_->morphs[id];
		}
		break;
	case NodeClass::Unknown:
		if (static_cast<std::size_t>(id) < dict_->unk_dict.morphs.size()) {
			morph = dict_->unk_dict.morphs[id];
		}
		break;
	case NodeClass::Dummy:
	case NodeClass::User:
	default:
		// Use default morph (zeros)
		break;
	}

	Node *node = node_pool_.get();
	node->set_id(id);
	node->set_position(position);
	node->set_start(start);
	node->set_class(node_class);
	node->set_cost(0);
	node->set_left_id(morph.left_id);
	node->set_right_id(morph.right_id);
	node->set_weight(morph.weight);
	node->set_surface(std::move(surface));
	node->set_prev(nullptr);

	// Calculate target position
	std::int32_t target_pos = pos;
	if (!node->surface().empty()) {
		// Count characters in surface using optimized function
		std::int32_t char_count = count_utf8_chars(node->surface());
		target_pos = pos + char_count;
	}

	if (target_pos < static_cast<std::int32_t>(node_list_.size())) {
		node_list_[target_pos].push_back(node);
	}
}

void Lattice::forward(LatticeMode mode)
{
	for (std::size_t i = 1; i < node_list_.size(); ++i) {
		auto &current_list = node_list_[i];

		for (std::size_t j = 0; j < current_list.size(); ++j) {
			Node *target = current_list[j];

			if (target->start() >= static_cast<std::int32_t>(node_list_.size())) {
				target->set_cost(MAXIMUM_COST);
				continue;
			}

			const auto &prev_list = node_list_[target->start()];
			if (prev_list.empty()) {
				target->set_cost(MAXIMUM_COST);
				continue;
			}

			for (std::size_t k = 0; k < prev_list.size(); ++k) {
				const Node *prev = prev_list[k];

				// Calculate connection cost
				std::int16_t connection_cost = 0;
				if (prev->node_class() != NodeClass::User &&
					target->node_class() != NodeClass::User) {
					connection_cost = dict_->connection.at(
						static_cast<std::size_t>(prev->right_id()),
						static_cast<std::size_t>(target->left_id()));
				}

				std::int64_t total_cost = static_cast<std::int64_t>(connection_cost) +
										  static_cast<std::int64_t>(target->weight()) +
										  static_cast<std::int64_t>(prev->cost());

				// Add search mode penalty
				if (mode != LatticeMode::Normal) {
					total_cost += additional_cost(prev);
				}

				// Clamp to maximum
				if (total_cost > MAXIMUM_COST) {
					total_cost = MAXIMUM_COST;
				}

				// Update if this is the best path so far
				if (k == 0 || static_cast<std::int32_t>(total_cost) < target->cost()) {
					target->set_cost(static_cast<std::int32_t>(total_cost));
					target->set_prev(prev);
				}
			}
		}
	}
}

void Lattice::backward(LatticeMode mode)
{
	output_.clear();

	if (node_list_.empty() || node_list_.back().empty()) {
		return;
	}

	// Collect nodes by tracing back from EOS
	std::vector<Node *> collected_nodes;
	const Node *current = node_list_.back()[0];

	while (current != nullptr) {
		if (mode != LatticeMode::Extended || current->node_class() != NodeClass::Unknown) {
			collected_nodes.push_back(const_cast<Node *>(current));
		}
		else {
			// Extended mode: break unknown words into characters
			const std::string &surface = current->surface();
			std::vector<Node *> char_nodes;

			// Iterate directly over UTF-8 string without conversion
			std::int32_t byte_pos = 0;
			std::int32_t surface_len = static_cast<std::int32_t>(surface.length());

			while (byte_pos < surface_len) {
				UChar32 codepoint;
				std::int32_t char_start_byte = byte_pos;
				U8_NEXT(reinterpret_cast<const uint8_t *>(surface.data()), byte_pos, surface_len, codepoint);

				if (codepoint >= 0) {
					std::int32_t char_byte_len = byte_pos - char_start_byte;

					Node *char_node = node_pool_.get();
					char_node->set_id(current->id());
					char_node->set_start(current->position() + char_start_byte);
					char_node->set_class(NodeClass::Dummy);
					char_node->set_surface(surface.substr(char_start_byte, char_byte_len));
					char_node->set_position(current->position() + char_start_byte);

					char_nodes.push_back(char_node);
				}
			}

			// Add character nodes in forward order
			for (auto it = char_nodes.begin(); it != char_nodes.end(); ++it) {
				collected_nodes.push_back(*it);
			}
		}

		current = current->prev();
	}

	// Reverse the collected nodes to get forward order (BOS to EOS)
	for (auto it = collected_nodes.rbegin(); it != collected_nodes.rend(); ++it) {
		Node *node = *it;
		output_.push_back(node);
	}
}

void Lattice::export_dot(std::ostream &output) const
{
	// Create set of best path nodes for highlighting
	ankerl::unordered_dense::set<const Node *> best_nodes;
	for (const Node *node: output_) {
		best_nodes.insert(node);
	}

	// Collect all edges
	struct Edge {
		const Node *from;
		const Node *to;
	};
	std::vector<Edge> edges;

	for (std::size_t i = 1; i < node_list_.size(); ++i) {
		for (const Node *to_node: node_list_[i]) {
			if (to_node->node_class() == NodeClass::Unknown &&
				best_nodes.find(to_node) == best_nodes.end()) {
				continue;// Skip unknown nodes not in best path
			}

			if (to_node->start() < static_cast<std::int32_t>(node_list_.size())) {
				for (const Node *from_node: node_list_[to_node->start()]) {
					if (from_node->node_class() == NodeClass::Unknown &&
						best_nodes.find(from_node) == best_nodes.end()) {
						continue;
					}

					edges.push_back({from_node, to_node});
				}
			}
		}
	}

	// Output DOT format
	output << "graph lattice {\n";
	output << "dpi=48;\n";
	output << "graph [style=filled, splines=true, overlap=false, fontsize=30, rankdir=LR]\n";
	output << "edge [fontname=Helvetica, fontcolor=red, color=\"#606060\"]\n";
	output << "node [shape=box, style=filled, fillcolor=\"#e8e8f0\", fontname=Helvetica]\n";

	// Output nodes
	for (std::size_t i = 0; i < node_list_.size(); ++i) {
		for (const Node *node: node_list_[i]) {
			if (node->node_class() == NodeClass::Unknown &&
				best_nodes.find(node) == best_nodes.end()) {
				continue;
			}

			std::string surface = node->surface();
			if (node->is_bos_eos()) {
				surface = (i == 0) ? "BOS" : "EOS";
			}

			std::string pos = pos_feature(node);
			bool is_best = best_nodes.find(node) != best_nodes.end();

			if (is_best) {
				output << fmt::format("  \"{}\" [label=\"{}\\n{}\\n{}\",shape=ellipse, peripheries=2];\n",
									  reinterpret_cast<uintptr_t>(node), surface, pos, node->weight());
			}
			else if (node->node_class() != NodeClass::Unknown) {
				output << fmt::format("  \"{}\" [label=\"{}\\n{}\\n{}\"];\n",
									  reinterpret_cast<uintptr_t>(node), surface, pos, node->weight());
			}
		}
	}

	// Output edges
	for (const auto &edge: edges) {
		std::int16_t connection_cost = 0;
		if (edge.from->node_class() != NodeClass::User &&
			edge.to->node_class() != NodeClass::User) {
			connection_cost = dict_->connection.at(
				static_cast<std::size_t>(edge.from->right_id()),
				static_cast<std::size_t>(edge.to->left_id()));
		}

		bool from_best = best_nodes.find(edge.from) != best_nodes.end();
		bool to_best = best_nodes.find(edge.to) != best_nodes.end();

		if (from_best && to_best) {
			output << fmt::format("  \"{}\" -- \"{}\" [label=\"{}\", style=bold, color=blue, fontcolor=blue];\n",
								  reinterpret_cast<uintptr_t>(edge.from),
								  reinterpret_cast<uintptr_t>(edge.to),
								  connection_cost);
		}
		else {
			output << fmt::format("  \"{}\" -- \"{}\" [label=\"{}\"];\n",
								  reinterpret_cast<uintptr_t>(edge.from),
								  reinterpret_cast<uintptr_t>(edge.to),
								  connection_cost);
		}
	}

	output << "}\n";
}

void Lattice::clear()
{
	// Return all nodes to pool
	for (auto &node_vec: node_list_) {
		for (Node *node: node_vec) {
			node_pool_.put(node);
		}
		node_vec.clear();
	}

	for (Node *node: output_) {
		node_pool_.put(node);
	}

	node_list_.clear();
	output_.clear();
}

std::string Lattice::to_string() const
{
	std::string result;

	for (std::size_t i = 0; i < node_list_.size(); ++i) {
		result += fmt::format("[{}] :\n", i);
		for (const Node *node: node_list_[i]) {
			result += fmt::format("  ID:{} Class:{} Surface:'{}' Cost:{}\n",
								  node->id(), kagome::tokenizer::lattice::to_string(node->node_class()),
								  node->surface(), node->cost());
		}
		result += "\n";
	}

	return result;
}

std::int32_t Lattice::additional_cost(const Node *node) const
{
	if (!node || node->surface().empty()) {
		return 0;
	}

	// Count Unicode characters using optimized function
	std::int32_t char_count = count_utf8_chars(node->surface());

	if (char_count > SEARCH_MODE_KANJI_LENGTH && is_kanji_only(node->surface())) {
		return (char_count - SEARCH_MODE_KANJI_LENGTH) * SEARCH_MODE_KANJI_PENALTY;
	}

	if (char_count > SEARCH_MODE_OTHER_LENGTH) {
		return (char_count - SEARCH_MODE_OTHER_LENGTH) * SEARCH_MODE_OTHER_PENALTY;
	}

	return 0;
}

bool Lattice::is_kanji_only(std::string_view str) const
{
	if (str.empty()) {
		return false;
	}

	// Iterate directly over UTF-8 string without conversion
	std::int32_t byte_pos = 0;
	std::int32_t str_len = static_cast<std::int32_t>(str.length());
	bool found_char = false;

	while (byte_pos < str_len) {
		UChar32 codepoint;
		U8_NEXT(reinterpret_cast<const uint8_t *>(str.data()), byte_pos, str_len, codepoint);

		if (codepoint < 0) {
			// Invalid UTF-8
			return false;
		}

		if (!u_hasBinaryProperty(codepoint, UCHAR_IDEOGRAPHIC)) {
			return false;
		}

		found_char = true;
	}

	return found_char;
}

std::string Lattice::pos_feature(const Node *node) const
{
	std::vector<std::string> features;

	switch (node->node_class()) {
	case NodeClass::Known:
		if (static_cast<std::size_t>(node->id()) < dict_->pos_table.pos_entries.size()) {
			const auto &pos_ids = dict_->pos_table.pos_entries[node->id()];
			for (auto pos_id: pos_ids) {
				if (pos_id < dict_->pos_table.name_list.size() &&
					dict_->pos_table.name_list[pos_id] != "*") {
					features.push_back(dict_->pos_table.name_list[pos_id]);
				}
			}
		}
		break;
	case NodeClass::Unknown: {
		auto start_it = dict_->contents_meta.find(
			std::string(dict::POS_START_INDEX));
		auto hierarchy_it = dict_->contents_meta.find(
			std::string(dict::POS_HIERARCHY));

		std::size_t start = (start_it != dict_->contents_meta.end())
								? start_it->second
								: 0;
		std::size_t hierarchy = (hierarchy_it != dict_->contents_meta.end())
									? hierarchy_it->second
									: 1;
		std::size_t end = start + hierarchy;

		if (static_cast<std::size_t>(node->id()) < dict_->unk_dict.contents.size()) {
			const auto &feature = dict_->unk_dict.contents[node->id()];
			if (start < end && end <= feature.size()) {
				for (std::size_t i = start; i < end; ++i) {
					if (feature[i] != "*") {
						features.push_back(feature[i]);
					}
				}
			}
		}
		break;
	}
	case NodeClass::User:
		if (user_dict_ && static_cast<std::size_t>(node->id()) < user_dict_->contents.size()) {
			features.push_back(user_dict_->contents[node->id()].pos);
		}
		break;
	case NodeClass::Dummy:
	default:
		break;
	}

	if (features.empty()) {
		return "---";
	}

	std::string result = features[0];
	for (std::size_t i = 1; i < features.size(); ++i) {
		result += "/" + features[i];
	}

	return result;
}

std::unique_ptr<Lattice>
create_lattice(std::shared_ptr<dict::Dict> dictionary,
			   std::shared_ptr<dict::UserDict> user_dictionary)
{
	return std::make_unique<Lattice>(std::move(dictionary), std::move(user_dictionary));
}

}// namespace kagome::tokenizer::lattice