#ifndef SSCTESTVISUALRENDERING_H_
#define SSCTESTVISUALRENDERING_H_


/**Unit tests for class ssc::VisualRendering
 */
class TestVisualRendering : public CppUnit::TestFixture
{
public:
	void setUp();
	void tearDown();
	void testInitialize();
	void test_3D_Tool();
	void test_ACS_3D_Tool();
	void test_AnyDual_3D_Tool();
	void test_ACS_3Volumes();
	void test_AnyDual_3Volumes();
	
public:
	CPPUNIT_TEST_SUITE( TestVisualRendering );
		CPPUNIT_TEST( testInitialize );					
		CPPUNIT_TEST( test_3D_Tool );					
		CPPUNIT_TEST( test_ACS_3D_Tool );					
		CPPUNIT_TEST( test_AnyDual_3D_Tool );					
		CPPUNIT_TEST( test_ACS_3Volumes );					
		CPPUNIT_TEST( test_AnyDual_3Volumes );					
	CPPUNIT_TEST_SUITE_END();
private:
	class ViewsWindow* widget;
	std::vector<std::string> image;

};
CPPUNIT_TEST_SUITE_REGISTRATION( TestVisualRendering );

#endif /*TESTVISUALRENDERING_H_*/
