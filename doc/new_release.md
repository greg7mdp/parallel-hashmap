- update version in phmap_config.h
- update version in comment on top of CMakeLists.txt
- create the new release on github (tag `v1.3.8` - use semantic versioning)
- download the tar.gz from github, and use `sha256sum parallel-hashmap-v1.3.8.tar.gz` on linux to get the sha256

## conan

- fork and clone [conan-center repo](https://github.com/conan-io/conan-center-index)
     (or sync +  git pull)
- git checkout -b phmap_1.3.8
- update: `recipes/parallel-hashmap/all/conandata.yml` and `recipes/parallel-hashmap/config.yml`
- sudo pip install conan -U 
- cd recipes/parallel-hashmap/all
- conan create conanfile.py parallel-hashmap/1.3.8@ -pr:b=default -pr:h=default 
- create PR like [this](https://github.com/conan-io/conan-center-index/pull/13161)


## vcpkg

- fork and clone [vcpkg repo](https://github.com/microsoft/vcpkg)
     (or sync +  git pull)
- git checkout -b phmap_1.3.8
- update ports/parallel-hashmap/portfile.cmake and ports/parallel-hashmap/vcpkg.json

in windows, non-cygwin console

- set VCPKG_ROOT=
- vcpkg install parallel-hashmap --triplet x64-windows
- # update sha in portfile.cmake
- git diff
- git commit ...
- vcpkg x-add-version --all --overwrite-version ## (or ./vcpkg.exe --no-dry-run upgrade )
- git diff
- git commit ...
- git push origin phmap_1.3.8 
