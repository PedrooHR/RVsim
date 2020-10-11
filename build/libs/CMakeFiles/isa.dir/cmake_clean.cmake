file(REMOVE_RECURSE
  "libisa.a"
  "libisa.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/isa.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
