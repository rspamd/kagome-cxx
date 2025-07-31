#pragma once

#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <iostream>
#include <cstdint>

#include "kagome/tokenizer/lattice/node.hpp"
#include "kagome/dict/dict.hpp"

namespace kagome::tokenizer::lattice {

/// Tokenization modes for lattice processing
enum class LatticeMode : std::uint8_t {
	Normal = 1,
	Search = 2,
	Extended = 3
};

/// Memory pool for efficient node allocation
template<typename T>
class ObjectPool {
public:
	ObjectPool() = default;
	~ObjectPool()
	{
		clear();
	}

	/// Get an object from the pool (create new if empty)
	T *get()
	{
		if (pool_.empty()) {
			return new T{};
		}
		T *obj = pool_.back();
		pool_.pop_back();
		return obj;
	}

	/// Return an object to the pool
	void put(T *obj)
	{
		if (obj) {
			// Reset object state
			*obj = T{};
			pool_.push_back(obj);
		}
	}

	/// Clear all pooled objects
	void clear()
	{
		for (T *obj: pool_) {
			delete obj;
		}
		pool_.clear();
	}

private:
	std::vector<T *> pool_;
};

/// Lattice for morphological analysis using Viterbi algorithm
class Lattice {
public:
	/// Create a new lattice with dictionaries
	Lattice(std::shared_ptr<dict::Dict> dictionary,
			std::shared_ptr<dict::UserDict> user_dictionary = nullptr);

	~Lattice() = default;

	// Non-copyable but movable
	Lattice(const Lattice &) = delete;
	Lattice &operator=(const Lattice &) = delete;
	Lattice(Lattice &&) = default;
	Lattice &operator=(Lattice &&) = default;

	/// Build lattice from input text
	void build(std::string_view input);

	/// Run forward algorithm (Viterbi)
	void forward(LatticeMode mode);

	/// Run backward algorithm to extract best path
	void backward(LatticeMode mode);

	/// Export lattice as DOT graph for visualization
	void export_dot(std::ostream &output) const;

	/// Get the best path output
	[[nodiscard]] const std::vector<Node *> &output() const noexcept
	{
		return output_;
	}

	/// Get input text
	[[nodiscard]] const std::string &input() const noexcept
	{
		return input_;
	}

	/// Clear lattice and reset state
	void clear();

	/// Debug string representation
	[[nodiscard]] std::string to_string() const;

private:
	std::shared_ptr<dict::Dict> dict_;
	std::shared_ptr<dict::UserDict> user_dict_;

	/// Input text
	std::string input_;

	/// Lattice nodes organized by position
	std::vector<std::vector<Node *>> node_list_;

	/// Best path output
	std::vector<Node *> output_;

	/// Node memory pool
	static ObjectPool<Node> node_pool_;

	/// Add a node to the lattice
	void add_node(std::int32_t pos, std::int32_t id, std::int32_t position,
				  std::int32_t start, NodeClass node_class, std::string surface);

	/// Calculate additional cost for search mode
	[[nodiscard]] std::int32_t additional_cost(const Node *node) const;

	/// Check if string contains only kanji characters
	[[nodiscard]] bool is_kanji_only(std::string_view str) const;

	/// Get POS feature string for DOT output
	[[nodiscard]] std::string pos_feature(const Node *node) const;
};

/// Factory function for creating lattices
[[nodiscard]] std::unique_ptr<Lattice>
create_lattice(std::shared_ptr<dict::Dict> dictionary,
			   std::shared_ptr<dict::UserDict> user_dictionary = nullptr);

}// namespace kagome::tokenizer::lattice