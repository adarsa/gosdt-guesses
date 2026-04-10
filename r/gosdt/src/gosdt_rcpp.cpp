#include <Rcpp.h>

// Placeholder export until Dataset / Configuration wrappers are implemented.
// [[Rcpp::export]]
std::string gosdt_lib_version() {
  return std::string("gosdt R bindings 0.0.1 (libgosdt linked)");
}
