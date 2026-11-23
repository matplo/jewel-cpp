# Use the actual install prefix, including an override passed to cmake --install.
set(registry_dir "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/jewel-cpp/hepyy")
file(MAKE_DIRECTORY "${registry_dir}")
string(REPLACE "\\" "\\\\" prefix "${CMAKE_INSTALL_PREFIX}")
string(REPLACE "\"" "\\\"" prefix "${prefix}")
set(record [=[
{
  "schema_version": 2,
  "packages": {
    "jewel_cpp": {
      "@jewel_package_version@": {
        "version": "@jewel_package_version@",
        "prefix": "@prefix@",
        "include_dir": "@prefix@/include",
        "lib_dir": "@prefix@/lib",
        "cppyy_namespace": "jewel",
        "headers": ["jewel/event_generator.hpp"],
        "libraries": ["jewel_generator"],
        "depends_on": ["lhapdf", "pythia8"]
      }
    }
  }
}
]=])
string(CONFIGURE "${record}" registry @ONLY)
file(WRITE "${registry_dir}/registry.json" "${registry}")
