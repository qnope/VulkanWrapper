#include "VulkanWrapper/Utils/Error.h"
#include <gtest/gtest.h>

// Test base Exception class
TEST(ExceptionTest, BasicException) {
    try {
        throw vw::Exception("Test error message");
    } catch (const vw::Exception &e) {
        EXPECT_NE(std::string(e.what()).find("Test error message"),
                  std::string::npos);
        EXPECT_EQ(e.message(), "Test error message");
        EXPECT_NE(std::string(e.what()).find("ErrorTests.cpp"),
                  std::string::npos);
    }
}

TEST(ExceptionTest, ExceptionInheritance) {
    try {
        throw vw::Exception("Test message");
    } catch (const std::exception &e) {
        // Should be catchable as std::exception
        EXPECT_NE(std::string(e.what()).find("Test message"),
                  std::string::npos);
    }
}

// Test VulkanException
TEST(VulkanExceptionTest, VulkanErrorWithResult) {
    try {
        throw vw::VulkanException(vk::Result::eErrorOutOfDeviceMemory,
                                  "Failed to allocate buffer");
    } catch (const vw::VulkanException &e) {
        EXPECT_EQ(e.result(), vk::Result::eErrorOutOfDeviceMemory);
        EXPECT_EQ(e.message(), "Failed to allocate buffer");
        EXPECT_NE(e.result_string().find("OutOfDeviceMemory"),
                  std::string::npos);

        std::string what_str(e.what());
        EXPECT_NE(what_str.find("Failed to allocate buffer"),
                  std::string::npos);
        EXPECT_NE(what_str.find("OutOfDeviceMemory"), std::string::npos);
        EXPECT_NE(what_str.find("ErrorTests.cpp"), std::string::npos);
    }
}

TEST(VulkanExceptionTest, CheckVkThrows) {
    EXPECT_THROW(vw::check_vk(vk::Result::eErrorUnknown, "Test error"),
                 vw::VulkanException);
}

TEST(VulkanExceptionTest, CheckVkDoesNotThrowOnSuccess) {
    EXPECT_NO_THROW(vw::check_vk(vk::Result::eSuccess, "Test success"));
}

TEST(VulkanExceptionTest, CheckVkPairReturnsValue) {
    std::pair<vk::Result, int> result{vk::Result::eSuccess, 42};
    int value = vw::check_vk(result, "Test pair");
    EXPECT_EQ(value, 42);
}

TEST(VulkanExceptionTest, CheckVkPairThrowsOnError) {
    std::pair<vk::Result, int> result{vk::Result::eErrorUnknown, 42};
    EXPECT_THROW(vw::check_vk(result, "Test pair error"), vw::VulkanException);
}

// Test VMAException
TEST(VMAExceptionTest, VMAErrorWithResult) {
    try {
        throw vw::VMAException(VK_ERROR_OUT_OF_DEVICE_MEMORY,
                               "VMA allocation failed");
    } catch (const vw::VMAException &e) {
        EXPECT_EQ(e.result(), VK_ERROR_OUT_OF_DEVICE_MEMORY);
        EXPECT_EQ(e.message(), "VMA allocation failed");
        EXPECT_NE(e.result_string().find("OutOfDeviceMemory"),
                  std::string::npos);

        std::string what_str(e.what());
        EXPECT_NE(what_str.find("VMA allocation failed"), std::string::npos);
        EXPECT_NE(what_str.find("OutOfDeviceMemory"), std::string::npos);
    }
}

TEST(VMAExceptionTest, CheckVmaThrows) {
    EXPECT_THROW(vw::check_vma(VK_ERROR_OUT_OF_HOST_MEMORY, "Test VMA error"),
                 vw::VMAException);
}

TEST(VMAExceptionTest, CheckVmaDoesNotThrowOnSuccess) {
    EXPECT_NO_THROW(vw::check_vma(VK_SUCCESS, "Test VMA success"));
}

// Test FileException
TEST(FileExceptionTest, FileExceptionWithPath) {
    std::filesystem::path test_path = "/path/to/missing/file.txt";
    try {
        throw vw::FileException(test_path, "File not found");
    } catch (const vw::FileException &e) {
        EXPECT_EQ(e.path(), test_path);
        EXPECT_EQ(e.message(), "File not found");

        std::string what_str(e.what());
        EXPECT_NE(what_str.find("File not found"), std::string::npos);
        EXPECT_NE(what_str.find("/path/to/missing/file.txt"),
                  std::string::npos);
    }
}

// Test AssimpException
TEST(AssimpExceptionTest, AssimpExceptionWithError) {
    std::filesystem::path model_path = "/path/to/model.obj";
    try {
        throw vw::AssimpException("Invalid mesh format", model_path);
    } catch (const vw::AssimpException &e) {
        EXPECT_EQ(e.assimp_error(), "Invalid mesh format");
        EXPECT_EQ(e.path(), model_path);

        std::string what_str(e.what());
        EXPECT_NE(what_str.find("Failed to load model"), std::string::npos);
        EXPECT_NE(what_str.find("/path/to/model.obj"), std::string::npos);
        EXPECT_NE(what_str.find("Invalid mesh format"), std::string::npos);
    }
}

// Test LogicException factory methods
TEST(LogicExceptionTest, OutOfRange) {
    try {
        throw vw::LogicException::out_of_range("InstanceId", 15, 10);
    } catch (const vw::LogicException &e) {
        std::string what_str(e.what());
        EXPECT_NE(what_str.find("InstanceId"), std::string::npos);
        EXPECT_NE(what_str.find("15"), std::string::npos);
        EXPECT_NE(what_str.find("10"), std::string::npos);
        EXPECT_NE(what_str.find("out of range"), std::string::npos);
    }
}

TEST(LogicExceptionTest, InvalidState) {
    try {
        throw vw::LogicException::invalid_state("Instance has been removed");
    } catch (const vw::LogicException &e) {
        std::string what_str(e.what());
        EXPECT_NE(what_str.find("Instance has been removed"),
                  std::string::npos);
    }
}

TEST(LogicExceptionTest, NullPointer) {
    try {
        throw vw::LogicException::null_pointer("BufferReference");
    } catch (const vw::LogicException &e) {
        std::string what_str(e.what());
        EXPECT_NE(what_str.find("BufferReference"), std::string::npos);
        EXPECT_NE(what_str.find("is null"), std::string::npos);
    }
}

// Test SDLException (basic test without SDL context)
TEST(SDLExceptionTest, SDLExceptionCreation) {
    try {
        throw vw::SDLException("SDL initialization failed");
    } catch (const vw::SDLException &e) {
        EXPECT_EQ(e.message(), "SDL initialization failed");
        std::string what_str(e.what());
        EXPECT_NE(what_str.find("SDL initialization failed"),
                  std::string::npos);
    }
}

TEST(SDLExceptionTest, CheckSdlBoolThrows) {
    EXPECT_THROW(vw::check_sdl(false, "Test SDL error"), vw::SDLException);
}

TEST(SDLExceptionTest, CheckSdlBoolDoesNotThrowOnSuccess) {
    EXPECT_NO_THROW(vw::check_sdl(true, "Test SDL success"));
}

TEST(SDLExceptionTest, CheckSdlPointerReturnsValue) {
    int value = 42;
    int *ptr = vw::check_sdl(&value, "Test pointer");
    EXPECT_EQ(ptr, &value);
}

TEST(SDLExceptionTest, CheckSdlPointerThrowsOnNull) {
    int *null_ptr = nullptr;
    EXPECT_THROW(vw::check_sdl(null_ptr, "Test null pointer"),
                 vw::SDLException);
}

// Test exception hierarchy catching
TEST(ExceptionHierarchyTest, CatchVulkanAsException) {
    try {
        throw vw::VulkanException(vk::Result::eErrorUnknown, "Test");
    } catch (const vw::Exception &e) {
        SUCCEED();
    }
}

TEST(ExceptionHierarchyTest, CatchSDLAsException) {
    try {
        throw vw::SDLException("Test");
    } catch (const vw::Exception &e) {
        SUCCEED();
    }
}

TEST(ExceptionHierarchyTest, CatchVMAAsException) {
    try {
        throw vw::VMAException(VK_ERROR_UNKNOWN, "Test");
    } catch (const vw::Exception &e) {
        SUCCEED();
    }
}

TEST(ExceptionHierarchyTest, CatchFileAsException) {
    try {
        throw vw::FileException("/path", "Test");
    } catch (const vw::Exception &e) {
        SUCCEED();
    }
}

TEST(ExceptionHierarchyTest, CatchAssimpAsException) {
    try {
        throw vw::AssimpException("error", "/path");
    } catch (const vw::Exception &e) {
        SUCCEED();
    }
}

TEST(ExceptionHierarchyTest, CatchLogicAsException) {
    try {
        throw vw::LogicException("Test");
    } catch (const vw::Exception &e) {
        SUCCEED();
    }
}

// Test source location capture
TEST(SourceLocationTest, LocationCaptured) {
    try {
        throw vw::Exception("Test");
    } catch (const vw::Exception &e) {
        EXPECT_GT(e.location().line(), 0u);
        EXPECT_NE(std::string(e.location().file_name()).find("ErrorTests.cpp"),
                  std::string::npos);
    }
}

TEST(SourceLocationTest, CheckVkCapturesCallSite) {
    try {
        vw::check_vk(vk::Result::eErrorUnknown,
                     "Test"); // Line where check_vk is called
    } catch (const vw::VulkanException &e) {
        // The location should point to this file, not Error.h
        std::string what_str(e.what());
        EXPECT_NE(what_str.find("ErrorTests.cpp"), std::string::npos);
    }
}
