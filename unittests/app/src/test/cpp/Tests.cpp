//
// Created by Daosheng Mu on 1/9/21.
//

#include <gtest/gtest.h> // TODO: update to master
#include "Vector3d.h"
#include "Matrix4x4.h"

using namespace gfx_math;

TEST(TestVector3D, basic) {
  Vector3D<float> a(1, 0, 0);

  float res = a.DotProduct(Vector3D<float>(1, 0, 0));
  ASSERT_EQ(res, 1.0f);
}

TEST(TestVector3D, rayIntersection) {
  Vector3Df ray(1, 0, 0);
  Vector3Df p0(-2, 0, 0);
  Vector3Df planeNrm(-1, 0, 0);
  float planeDist = 10.0f;
  Vector3Df interPoint;

  bool result = RayIntersectWithPlane(ray, p0, planeNrm, planeDist, interPoint);
  ASSERT_TRUE(result);

  result = PointIntersectWithPlane(interPoint, planeNrm, planeDist);
  ASSERT_FALSE(result);

  float dist = PointDistanceWithPlane(interPoint + Vector3Df(1, 0, 0), planeNrm, planeDist);
  ASSERT_EQ(dist, 1.0f);

  std::vector<Vector3Df> interPointList;
  result = RayIntersectWithSphere(ray, Vector3Df(-10, 0, 0), Vector3Df(0,0,0), 5.0f, interPointList);
  ASSERT_TRUE(result);
  ASSERT_EQ(interPointList.size(), 2);

  interPointList.clear();
  result = RayIntersectWithSphere(ray, p0, Vector3Df(0,0,0), 5.0f, interPointList);
  ASSERT_TRUE(result);
  ASSERT_EQ(interPointList.size(), 1);

  Vector3Df p;
  result = RayIntersectWithTriangle(Vector3Df(0,0,-1), p0, Vector3Df(-10,-1,-5), Vector3Df(10,-1,-5), Vector3Df(5,1,-5), p);
  ASSERT_TRUE(result);
  ASSERT_EQ(p, Vector3Df(-2, 0, -5));

  result = RayIntersectWithAABBox(ray, p0, Vector3Df(2, -2, -2), Vector3Df(6, 2, 2), p);
  ASSERT_TRUE(result);
  ASSERT_EQ(p, Vector3Df(2, 0, 0));

  result = SphereIntersectWithAABBox(Vector3Df(0,0,0), 5.0f, Vector3Df(2, -2, -2), Vector3Df(6, 2, 2));
  ASSERT_TRUE(result);

  result = AABBoxIntersectWithAABBox(Vector3Df(0, 0, -2), Vector3Df(4, 4, 2), Vector3Df(2, -2, -2), Vector3Df(6, 2, 2));
  ASSERT_TRUE(result);
}

TEST(TestVector3D, reflection) {
  Vector3Df p = ReflectionVector(Vector3Df(0.707, 0.707, 0), Vector3Df(0, 1, 0));
  ASSERT_EQ(p, Vector3Df(-0.707, 0.707, 0));
}

TEST(TestMatrix4x4, cameraMatrix) {
  Matrix4x4<float> matA;
  Matrix4x4<float> matB;
  matA.Identity();
  ASSERT_EQ(matA, matB);

  Matrix4x4<float> matC = matA * matB;
  ASSERT_EQ(matC, matA);
}

TEST(TestMatrix4x4, basic) {
  Matrix4x4f matC;

  matC = Matrix4x4<float>::LookAtMatrix(Vector3D<float>(0, 0.5, 0.5),
                                        Vector3D<float>(0,0.2,-1),
                                        Vector3D<float>(0,1,0));

  Matrix4x4<float> matD = Matrix4x4<float>::TargetTo(Vector3D<float>(0, 0.5, 0.5),                                                                                           Vector3D<float>(0,0.2,-1),
                                                     Vector3D<float>(0,1,0));
  ASSERT_NE(matC, matD); // TODO: We haven't known how to test it.

  Matrix4x4<float> matE = Matrix4x4<float>::FPSView(Vector3D<float>(0, 0.5, 0.5), 0.1, 0.2);

  Matrix4x4<float> matF = Matrix4x4<float>::Arcball(Vector3D<float>(0, 0.5, 0.5), Quaternion<float>(1,1,0,1), Vector3D<float>(1.0, 0.5, 0.5));
  ASSERT_NE(matE, matF); // TODO: We haven't known how to test it.
}
