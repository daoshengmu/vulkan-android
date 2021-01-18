//
// Created by Daosheng Mu on 1/10/21.
//

#include <jni.h>
#include <string>

#include "GTestRunner.hpp"

extern "C" {
JNIEXPORT jlong JNICALL Java_com_example_vulkanTests_GTestRunner_create(JNIEnv *env, jobject obj, jstring jtempDir) {
  const char *ctempDir = env->GetStringUTFChars(jtempDir, 0);
  std::string tempDir = ctempDir;
  env->ReleaseStringUTFChars(jtempDir, ctempDir);

  return (jlong)new GTestRunner(tempDir);
}

JNIEXPORT void JNICALL Java_com_example_vulkanTests_GTestRunner_destroy(JNIEnv *env, jobject obj, jlong cptr) {
  delete (reinterpret_cast<GTestRunner*>(cptr));
}

JNIEXPORT jboolean JNICALL Java_com_example_vulkanTests_GTestRunner_run(JNIEnv *env, jobject obj, jlong cptr, jstring jgtestArg, jobject joutput) {
  GTestRunner *gtestRunner= reinterpret_cast<GTestRunner*>(cptr);

  const char *cgtestArg = env->GetStringUTFChars(jgtestArg, 0);
  std::string gtestArg = cgtestArg;
  env->ReleaseStringUTFChars(jgtestArg, cgtestArg);

  std::string output;
  jboolean passed = gtestRunner->run(gtestArg, output);
  jclass clazz = env->GetObjectClass(joutput);
  // set test lists to the output string.
  jmethodID methodId = env->GetMethodID(clazz, "setValue", "(Ljava/lang/Object;)V");
  env->CallVoidMethod(joutput, methodId, env->NewStringUTF(output.c_str()));

  return passed;
}

}
