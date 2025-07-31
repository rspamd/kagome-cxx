#pragma once

#include <string>
#include <cstdint>
#include <limits>

namespace kagome::tokenizer::lattice {

/// Special node ID for BOS (Beginning of Sentence) and EOS (End of Sentence)
constexpr std::int32_t BOS_EOS_ID = -1;

/// Node classification for lattice processing
enum class NodeClass : std::uint8_t {
	/// Dummy node (BOS/EOS)
	Dummy = 0,
	/// Known word from system dictionary
	Known = 1,
	/// Unknown word
	Unknown = 2,
	/// User dictionary word
	User = 3
};

/// Convert NodeClass to string representation
constexpr const char *to_string(NodeClass cls) noexcept
{
	switch (cls) {
	case NodeClass::Dummy:
		return "DUMMY";
	case NodeClass::Known:
		return "KNOWN";
	case NodeClass::Unknown:
		return "UNKNOWN";
	case NodeClass::User:
		return "USER";
	default:
		return "INVALID";
	}
}

/// Lattice node representing a morphological unit in the processing graph
class Node {
public:
	/// Default constructor
	Node() = default;

	/// Construct a node with all parameters
	Node(std::int32_t id, std::int32_t position, std::int32_t start,
		 NodeClass node_class, std::int32_t cost, std::int16_t left_id,
		 std::int16_t right_id, std::int16_t weight, std::string surface)
		: id_(id), position_(position), start_(start), class_(node_class),
		  cost_(cost), left_id_(left_id), right_id_(right_id), weight_(weight),
		  surface_(std::move(surface)), prev_(nullptr)
	{
	}

	/// Copy and move constructors/assignment
	Node(const Node &) = default;
	Node &operator=(const Node &) = default;
	Node(Node &&) = default;
	Node &operator=(Node &&) = default;

	~Node() = default;

	// Accessors
	[[nodiscard]] std::int32_t id() const noexcept
	{
		return id_;
	}
	[[nodiscard]] std::int32_t position() const noexcept
	{
		return position_;
	}
	[[nodiscard]] std::int32_t start() const noexcept
	{
		return start_;
	}
	[[nodiscard]] NodeClass node_class() const noexcept
	{
		return class_;
	}
	[[nodiscard]] std::int32_t cost() const noexcept
	{
		return cost_;
	}
	[[nodiscard]] std::int16_t left_id() const noexcept
	{
		return left_id_;
	}
	[[nodiscard]] std::int16_t right_id() const noexcept
	{
		return right_id_;
	}
	[[nodiscard]] std::int16_t weight() const noexcept
	{
		return weight_;
	}
	[[nodiscard]] const std::string &surface() const noexcept
	{
		return surface_;
	}
	[[nodiscard]] const Node *prev() const noexcept
	{
		return prev_;
	}

	// Mutators
	void set_id(std::int32_t id) noexcept
	{
		id_ = id;
	}
	void set_position(std::int32_t position) noexcept
	{
		position_ = position;
	}
	void set_start(std::int32_t start) noexcept
	{
		start_ = start;
	}
	void set_class(NodeClass cls) noexcept
	{
		class_ = cls;
	}
	void set_cost(std::int32_t cost) noexcept
	{
		cost_ = cost;
	}
	void set_left_id(std::int16_t left_id) noexcept
	{
		left_id_ = left_id;
	}
	void set_right_id(std::int16_t right_id) noexcept
	{
		right_id_ = right_id;
	}
	void set_weight(std::int16_t weight) noexcept
	{
		weight_ = weight;
	}
	void set_surface(std::string surface)
	{
		surface_ = std::move(surface);
	}
	void set_prev(const Node *prev) noexcept
	{
		prev_ = prev;
	}

	/// Reset node to default state (for object pooling)
	void reset() noexcept
	{
		id_ = 0;
		position_ = 0;
		start_ = 0;
		class_ = NodeClass::Dummy;
		cost_ = 0;
		left_id_ = 0;
		right_id_ = 0;
		weight_ = 0;
		surface_.clear();
		prev_ = nullptr;
	}

	/// Check if this is a BOS/EOS node
	[[nodiscard]] bool is_bos_eos() const noexcept
	{
		return id_ == BOS_EOS_ID;
	}

private:
	/// Unique ID of this morpheme in the dictionary
	std::int32_t id_ = 0;

	/// Byte position in the original input
	std::int32_t position_ = 0;

	/// Character (rune) position in the original input
	std::int32_t start_ = 0;

	/// Node classification
	NodeClass class_ = NodeClass::Dummy;

	/// Accumulated cost from BOS to this node
	std::int32_t cost_ = 0;

	/// Left context ID for connection cost calculation
	std::int16_t left_id_ = 0;

	/// Right context ID for connection cost calculation
	std::int16_t right_id_ = 0;

	/// Base cost/weight of this morpheme
	std::int16_t weight_ = 0;

	/// Surface string (the actual text)
	std::string surface_;

	/// Previous node in the best path (for backtracking)
	const Node *prev_ = nullptr;
};

}// namespace kagome::tokenizer::lattice