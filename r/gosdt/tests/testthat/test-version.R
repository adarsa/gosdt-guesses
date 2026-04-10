test_that("gosdt_lib_version returns non-empty string", {
  v <- gosdt_lib_version()
  expect_type(v, "character")
  expect_true(nzchar(v))
})
