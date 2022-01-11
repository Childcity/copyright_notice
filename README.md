# Update Copyright Notice
### Ensures that files in a project under Git have a consistent copyright notice.

```
Usage: copyright_notice.exe [options] [paths...]

Options:
  -?, -h, --help                                Displays help on commandline
                                                options.
  --help-all                                    Displays help including Qt
                                                specific options.
  -v, --version                                 Displays version information.
  --component <name>                            Add or replace software
                                                component mention.
  --update-copyright                            Add or update copyright field
                                                (Year, Company, etc...).
  --update-filename                             Add or fix 'File' field.
  --update-authors                              Add or update author list.
  --update-authors-only-if-empty                Update author list only if this
                                                list is empty in author field
                                                (edited by someone else).
  --max-blame-authors-to-start-update <number>  Update author list only if
                                                blame authors <= some limit.
  --dont-skip-broken-merges                     Do not skip broken merge
                                                commits.
  --static-config <path>                        Json configuration file with
                                                static configuration.
  --dry                                         Do not modify files, print to
                                                stdout instead.
  --verbose                                     Print verbose output.

Arguments:
  file_or_dir                                   File or directory to process.
  ```

#### The tool has static configuration file
```json
{
  "author_aliases": {
    "bill.gates": "Bill Gates",
    "Amazone\\jeff.bezos": "Jeff Bezos",
    "Zuckerberg Mark": "Mark Zuckerberg"
  },
  "copyright_field_template": "(c) %CURRENT_YEAR%, Inc. All Rights Reserved.",
  "excluded_path_sections": [
    "/3rdparty/",
    "ext"
  ]
}
```
- `author_aliases` represent aliases for authors.
- `copyright_field_template` is a template for 'copyright field' in header.
  - NOTE: `%CURRENT_YEAR%` will be exchanged by current system year.
- `excluded_path_sections` represent parts of paths, that will be excluded when searching repository.

## Building using CMake
```shell
$ sudo apt install libssl-dev # libgit2 required OpenSSL.
$ sudo apt install qt5-default # this tool requires Qt5.

$ git clone https://github.com/Childcity/copyright_notice.git --recurse-submodules
$ cd copyright_notice
$ mkdir build- && cd build-
# To build using LibGit2 as git archive parser
  $ cmake -DUSE_LIBGIT2 -DCMAKE_BUILD_TYPE:STRING=Release ..
# Or to build without using LibGit2 as git archive parser
  $ cmake -DCMAKE_BUILD_TYPE:STRING=Release ..
$ cmake --build . --config Release -- -j
```