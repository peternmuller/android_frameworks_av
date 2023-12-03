/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include "android/hardware_buffer.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "system/graphics.h"
#include "ui/GraphicBuffer.h"
#include "util/EglDisplayContext.h"
#include "util/EglProgram.h"
#include "util/EglSurfaceTexture.h"
#include "util/EglUtil.h"
#include "utils/Errors.h"

namespace android {
namespace companion {
namespace virtualcamera {
namespace {

using ::testing::Eq;
using ::testing::NotNull;

constexpr int kWidth = 64;
constexpr int kHeight = 64;
constexpr char kGlExtYuvTarget[] = "GL_EXT_YUV_target";

uint8_t getY(const android_ycbcr& ycbcr, const int x, const int y) {
    uint8_t* yPtr = reinterpret_cast<uint8_t*>(ycbcr.y);
    return *(yPtr + ycbcr.ystride * y + x);
}

uint8_t getCb(const android_ycbcr& ycbcr, const int x, const int y) {
    uint8_t* cbPtr = reinterpret_cast<uint8_t*>(ycbcr.cb);
    return *(cbPtr + ycbcr.cstride * (y / 2) + (x / 2) * ycbcr.chroma_step);
}

uint8_t getCr(const android_ycbcr& ycbcr, const int x, const int y) {
    uint8_t* crPtr = reinterpret_cast<uint8_t*>(ycbcr.cr);
    return *(crPtr + ycbcr.cstride * (y / 2) + (x / 2) * ycbcr.chroma_step);
}

TEST(EglDisplayContextTest, SuccessfulInitialization) {
  EglDisplayContext displayContext;

  EXPECT_TRUE(displayContext.isInitialized());
}

class EglTest : public ::testing::Test {
public:
  void SetUp() override {
      ASSERT_TRUE(mEglDisplayContext.isInitialized());
      ASSERT_TRUE(mEglDisplayContext.makeCurrent());
  }

private:
  EglDisplayContext mEglDisplayContext;
};

TEST_F(EglTest, EglTestPatternProgramSuccessfulInit) {
  EglTestPatternProgram eglTestPatternProgram;

  // Verify the shaders compiled and linked successfully.
  EXPECT_TRUE(eglTestPatternProgram.isInitialized());
}

TEST_F(EglTest, EglTextureProgramSuccessfulInit) {
  if (!isGlExtensionSupported(kGlExtYuvTarget)) {
      GTEST_SKIP() << "Skipping test because of missing required GL extension " << kGlExtYuvTarget;
  }

  EglTextureProgram eglTextureProgram;

  // Verify the shaders compiled and linked successfully.
  EXPECT_TRUE(eglTextureProgram.isInitialized());
}

TEST_F(EglTest, EglSurfaceTextureBlackAfterInit) {
  if (!isGlExtensionSupported(kGlExtYuvTarget)) {
      GTEST_SKIP() << "Skipping test because of missing required GL extension " << kGlExtYuvTarget;
  }

  EglSurfaceTexture surfaceTexture(kWidth, kHeight);
  surfaceTexture.updateTexture();
  sp<GraphicBuffer> buffer = surfaceTexture.getCurrentBuffer();

  ASSERT_THAT(buffer, NotNull());
  const int width = buffer->getWidth();
  const int height = buffer->getHeight();
  ASSERT_THAT(width, Eq(kWidth));
  ASSERT_THAT(height, Eq(kHeight));

  android_ycbcr ycbcr;
  status_t ret = buffer->lockYCbCr(AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, &ycbcr);
  ASSERT_THAT(ret, Eq(NO_ERROR));
  for (int i = 0; i < width; ++i) {
      for (int j = 0; j < height; ++j) {
          EXPECT_THAT(getY(ycbcr, i, j), Eq(0x00));
          EXPECT_THAT(getCb(ycbcr, i, j), Eq(0x7f));
          EXPECT_THAT(getCr(ycbcr, i, j), Eq(0x7f));
      }
  }

  buffer->unlock();
}

}  // namespace
}  // namespace virtualcamera
}  // namespace companion
}  // namespace android
