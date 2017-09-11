#include "gtest/gtest.h"
#include <vector>
#include <Eigen/Geometry>

#include "minifem.h"

//----------------------------------------------------------------------------
// Helper 
//----------------------------------------------------------------------------
template<typename DerivedA, typename DerivedB>
bool allclose(const Eigen::DenseBase<DerivedA>& a,
    const Eigen::DenseBase<DerivedB>& b,
    const typename DerivedA::RealScalar& rtol
    = Eigen::NumTraits<typename DerivedA::RealScalar>::dummy_precision(),
    const typename DerivedA::RealScalar& atol
    = Eigen::NumTraits<typename DerivedA::RealScalar>::epsilon())
  {
  return ((a.derived() - b.derived()).array().abs()
    <= (atol + rtol * b.derived().array().abs())).all();
  }

//----------------------------------------------------------------------------
// Integration rules
//----------------------------------------------------------------------------
TEST(miniGaussRule, prismFloat)
{
  mini::GaussRule<float, mini::prism> GR;
  Eigen::Vector3f vec0;
  vec0 << -1./sqrt(3.), -1./sqrt(3.), -1./sqrt(3.);
  Eigen::Vector3f vec5;
  vec5 << 1./sqrt(3.), -1./sqrt(3.), 1./sqrt(3.);
  
  EXPECT_EQ((float)1., GR.weight());
  EXPECT_TRUE(allclose(vec0, GR.point(0)));
  EXPECT_TRUE(allclose(vec5, GR.point(5)));
}

TEST(miniGaussRule, prismDouble)
{
  mini::GaussRule<double, mini::prism> GR;
  Eigen::Vector3d vec0;
  vec0 << -1./sqrt(3.), -1./sqrt(3.), -1./sqrt(3.);
  Eigen::Vector3d vec5;
  vec5 << 1./sqrt(3.), -1./sqrt(3.), 1./sqrt(3.);
  
  EXPECT_EQ(1., GR.weight());
  EXPECT_TRUE(allclose(vec0, GR.point(0)));
  EXPECT_TRUE(allclose(vec5, GR.point(5)));
}

//----------------------------------------------------------------------------
// Node
//----------------------------------------------------------------------------
TEST(miniNode, dim2Double)
{
  mini::Node<double, 2> node;
  Eigen::Vector2d X; X << 1., sqrt(2.);
  Eigen::Vector2d u; u << -1.5, 0.01;
  node.setX(X);
  node.setu(u);
  EXPECT_TRUE(allclose(X, node.X()));
  EXPECT_TRUE(allclose(u, node.u()));
  EXPECT_TRUE(allclose(X+u, node.x()));
  node.setx(X);
  EXPECT_TRUE(allclose(Eigen::Vector2d::Zero(), node.u()));
}

TEST(miniNode, dim3Float)
{
  mini::Node<float, 3> node;
  Eigen::Vector3f X; X << 1., sqrt(2.), -0.05;
  Eigen::Vector3f u; u << -1.5, 0.01, 0.05;
  node.setX(X);
  node.setu(u);
  EXPECT_TRUE(allclose(X, node.X()));
  EXPECT_TRUE(allclose(u, node.u()));
  EXPECT_TRUE(allclose(X+u, node.x()));
  node.setx(X);
  EXPECT_TRUE(allclose(Eigen::Vector3f::Zero(), node.u()));
}

//----------------------------------------------------------------------------
// Element
//----------------------------------------------------------------------------
TEST(miniElement, C3D20Rdouble)
{
  //X is a dummy to construct the nodes
  Eigen::Matrix<double, 3, 22> dummy;
  dummy <<  -1, 0, 2, 2, 0, 0, 2, 2, 0, 1, 2, 1, 0, 1, 2, 1, 0, 0, 2, 2, 0, 5, 
            -1, 0, 0, 2, 2, 0, 0, 2, 2, 0, 1, 2, 1, 0, 1, 2, 1, 0, 0, 2, 2, 4, 
            -1, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 1, 1, 3;
  //element requires a storage for some results
  std::vector<mini::Element<double,3>::PointInfo> storage;
  //this vector of nodes is usually part of the FEM. it contains the global 
  //    nodes so I added a dummy node at the beginning and another at the end.
  std::vector<mini::Node<double, 3>> nodes;
  nodes.resize(22);
  for(int i = 0; i<22; i++)
    nodes.at(i).setX(dummy.col(i));
  std::vector<int> conn;
  for(int i = 0; i<20; i++)
    conn.push_back(i+1);
  double E = 5, nu = 0.3;
  mini::IsotropicLinear<double, 3> isoMat(E, nu);
  mini::C3D20R<mini::IsotropicLinear<double, 3>> testEl(nodes, isoMat);
  testEl.setConn(conn);
  //testing node access
  EXPECT_TRUE(allclose(Eigen::Matrix<double,  3, 1>::Zero(), testEl.getNode(0).X() ));  
  EXPECT_TRUE(allclose(dummy.col(20), testEl.getNode(19).X() )); 
  //testing for zero forces on undeformed element
  EXPECT_TRUE(allclose(Eigen::Matrix<double, 60, 1>::Zero(), testEl.F(storage)));
  //testing for zero foces on translated element
  for(int i = 0; i<22; i++)
    nodes.at(i).setu(Eigen::Vector3d::Ones());
  EXPECT_TRUE(allclose(Eigen::Matrix<double, 60, 1>::Zero(), testEl.F(storage), 1e-10, 1e-10));
  //testing for zero forces on rotated element
  Eigen::Matrix3d R;
  R = Eigen::AngleAxisd(1.5, Eigen::Vector3d::Ones().normalized());
  for(int i = 0; i<22; i++)
    nodes.at(i).setx(R * dummy.col(i));
  EXPECT_TRUE(allclose(Eigen::Matrix<double, 60, 1>::Zero(), testEl.F(storage), 1e-10, 1e-10));
  //testing for uniform stretch in the x direction
  double d = 2;
  for(int i = 0; i<22; i++)
    {
    Eigen::Vector3d temp; temp << .5*d*dummy(0,i), 0, 0;
    nodes.at(i).setu(temp);
    }
  //getting the total force on two of the faces
  double fx = 0, fy = 0;
  Eigen::MatrixXd f = testEl.F(storage);
  for(int i = 0; i<20; i++){
    if(testEl.getNode(i).X()(0) > 1.9)
      fx += f(i*3);
    if (testEl.getNode(i).X()(1) > 1.9)
      fy += f(i*3+1);
  }
  //Using plane strain we can get the correct result. 
  double E11 = .5*((2.+d)*(2.+d)-4.)/4.;
  double S11 = E11 * E*(1-nu)/(1+nu)/(1-2*nu);
  double S22 = E11 * E* nu/(1+nu)/(1-2*nu);
  double area = 4.;
  EXPECT_NEAR(storage.at(1).strain(0,0), E11 , 1e-10);
  EXPECT_NEAR(storage.at(1).stress(0,0), S11 , 1e-10);
  EXPECT_NEAR(fx, area*S11*(d+2.)/2., 1e-10);
  EXPECT_NEAR(fy, area*S22, 1e-10);
}

//----------------------------------------------------------------------------
// FEM
//----------------------------------------------------------------------------

TEST(miniFEM, patchC3D20Rdouble)
{
  mini::FEM<double, 3> testModel;
  double E = 5, nu = 0.3;
  mini::IsotropicLinear<double, 3> isoMat(E, nu);
  bool readSuccess = testModel.ReadAbaqusInp("../benchmark/input/patchC3D20R.inp", isoMat);
  ASSERT_TRUE(readSuccess);
  Eigen::VectorXd zerovec = Eigen::VectorXd::Zero(testModel.F().size());
  //there should be zero forces initially
  EXPECT_TRUE(allclose(testModel.F(), zerovec));
  //stretching in the y direction
  double d = 1;
  for(int i = 0; i<testModel.NumNodes(); i++)
    {
    mini::FEM<double, 3>::Node::VectorType U;
    U << 0 , d*testModel.getNode(i).X()(1), 0;
    testModel.setuNode(i, U);
    }
  //getting the total force on two of the faces
  double fx = 0, fy = 0;
  Eigen::MatrixXd f = testModel.F();
  for(int i = 0; i<testModel.NumNodes(); i++){
    if(testModel.getNode(i).X()(0) > 0.9)
      fx += f(i*3);
    if (testModel.getNode(i).X()(1) > 0.9)
      fy += f(i*3+1);
  }
  //Using plane strain we can get the correct result. 
  double E11 = .5*((1.+d)*(1.+d)-1.)/1.;
  double S11 = E11 * E*(1-nu)/(1+nu)/(1-2*nu);
  double S22 = E11 * E* nu/(1+nu)/(1-2*nu);
  double area = 1.;
  EXPECT_NEAR(fy, area*S11*(d+1.)/1., 1e-10);
  EXPECT_NEAR(fx, area*S22, 1e-10);
  /* disabled temporarily. I have to check if it is true for this element
  //rotating the entire model should give the same forces
  Eigen::Matrix3d R;
  R = Eigen::AngleAxisd(2*atan(1), Eigen::Vector3d::UnitZ());
  for(int i = 0; i<testModel.NumNodes(); i++)
    {
    mini::FEM<double, 3>::Node::VectorType U;
    U = R * testModel.getNode(i).x() - testModel.getNode(i).X();
    testModel.setuNode(i, U);
    }
  //however they are in different directions now
  fx = 0; fy = 0;
  f = testModel.F();
  for(int i = 0; i<testModel.NumNodes(); i++){
    if(testModel.getNode(i).x()(0) > -.1)
      fx += f(i*3);
    if (testModel.getNode(i).x()(1) > 0.9)
      fy += f(i*3+1);
  }
  EXPECT_NEAR(fy, area*S11*(d+1.)/1., 1e-10);
  EXPECT_NEAR(fx, area*S22, 1e-10);
  */
}






































