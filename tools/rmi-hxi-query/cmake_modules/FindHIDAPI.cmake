FIND_PATH                         (HIDAPI_INCLUDE_DIR NAMES hidapi/hidapi.h)
FIND_LIBRARY                      (HIDAPI_LIBRARIES NAMES hidapi-hidraw hidapi-libusb)

INCLUDE                           (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (HIDAPI DEFAULT_MSG HIDAPI_LIBRARIES HIDAPI_INCLUDE_DIR)
