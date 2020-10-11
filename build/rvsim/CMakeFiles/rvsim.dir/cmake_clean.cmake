file(REMOVE_RECURSE
  "rvsim"
  "rvsim.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/rvsim.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
