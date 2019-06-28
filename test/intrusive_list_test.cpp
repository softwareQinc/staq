#include "qasm/ast/detail/intrusive_list.hpp"
#include "qasm/ast/ast.hpp"

using namespace synthewareQ::qasm;

class int_node : public detail::intrusive_list_node<int_node> {
  public:
    int val_;

    int_node(int val) : val_(val) {}
    ~int_node() {}

    void do_on_insert(const int_node* parent) {}
};

void show_list(detail::intrusive_list<int_node>& list) {
  for (auto& node : list) {
    std::cout << node.val_ << " ";
  }
  std::cout << "\n";
}

int main() {
  auto root = new int_node(0);
  detail::intrusive_list<int_node> children;

  for (int i = 0; i < 5; i++) {
    children.push_back(root, new int_node(2*i));
  }

  show_list(children);

  auto it = children.begin();
  for (int i = 0; i < 5; i++) {
    it = children.insert(it, root, new int_node(2*i+1));
    it++;
  }

  show_list(children);

  for (it = children.begin(); it != children.end(); it++) {
    it = children.assign(it, root, new int_node(it->val_ + 10));
  }

  show_list(children);

  for (it = children.begin(); it != children.end(); it) {
    it = children.remove(it);
  }

  show_list(children);

  return 1;
}
