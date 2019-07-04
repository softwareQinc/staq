#include "qasm/ast/detail/intrusive_list.hpp"
#include "qasm/ast/ast.hpp"

using namespace synthewareQ::qasm;

class int_node : public detail::intrusive_list_node<int_node> {
public:
  int val_;
  int_node const* parent_;

  int_node(int val) : val_(val) {}
  ~int_node() {}

  void do_on_insert(const int_node* parent) { parent_ = parent; }
};

void show_list(detail::intrusive_list<int_node>& list) {
  for (auto& node : list) {
    std::cout << node.val_ << "(" << (node.parent_)->val_ << ") ";
  }
  std::cout << "\n";
}

int main() {
  auto root = new int_node(0);
  detail::intrusive_list<int_node> children;

  // Push back
  for (int i = 0; i < 5; i++) {
    children.push_back(root, new int_node(2*i+1));
  }
  show_list(children);

  // Insertion
  int i = 0;
  for (auto it = children.begin(); it != children.end(); it++) {
    children.insert(it, root, new int_node(2*i));
    i++;
  }
  show_list(children);

  // Splicing
  detail::intrusive_list<int_node> xs;
  auto subroot = &(*(std::next(children.begin())));
  for (int i = 1; i < 4; i++) {
    xs.push_back(subroot, new int_node(20*i));
  }
  show_list(xs);
  children.splice(std::next(children.begin(), 3), root, xs);
  show_list(children);

  // Assignment
  for (auto it = children.begin(); it != children.end(); it++) {
    it = children.assign(it, root, new int_node(it->val_ + 10));
  }
  show_list(children);

  // Deletion
  for (auto it = children.begin(); it != children.end(); it) {
    it = children.erase(it);
  }
  show_list(children);

  return 1;
}
