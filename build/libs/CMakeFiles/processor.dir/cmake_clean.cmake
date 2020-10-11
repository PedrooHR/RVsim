file(REMOVE_RECURSE
  "libprocessor.a"
  "libprocessor.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/processor.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
