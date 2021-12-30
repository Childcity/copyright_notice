#pragma once

#ifdef USE_LIBGIT2
#include "src/file_processor/git/libgit2_src/GitRepository.h"
#else
#include "src/file_processor/git/cmdgit_src/GitRepository.h"
#endif