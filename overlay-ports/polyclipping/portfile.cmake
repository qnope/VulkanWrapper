set(VCPKG_POLICY_EMPTY_PACKAGE enabled)

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/polyclippingConfig.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/polyclipping")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/FindCLIPPER.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/clipper")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/vcpkg-cmake-wrapper.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/clipper")
file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright" "Boost Software License 1.0")
