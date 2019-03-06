libdecsync_inc = include_directories(['.'])
libdecsync_lib = static_library(
  'libdecsync',
  [
    'Decsync.vala',
    'DirectoryMonitor.vala',
    'FileUtils.vala',
    'Log.vala',
    'OnEntryUpdateListener.vala',
    'Utils.vala'
  ],
  dependencies: [
    gee,
    json_glib
  ]
)

libdecsync = declare_dependency(
  link_with: libdecsync_lib,
  include_directories: libdecsync_inc
)
