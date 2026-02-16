#include <to_qasm_converter.hpp>
#include <fstream>
#include <tools_v1/parser/parser.hpp>

int main()
{
  std::ifstream fin("observable.qasm");
  tools_v1::ast::QASMify to_qasm;
  tools_v1::ast::ptr<tools_v1::ast::Program> p = tools_v1::parser::parse_stream(fin);
  std::cout << p << std::endl;
  p->accept(to_qasm);
  return 0;
}
