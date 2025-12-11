#include <gtest/gtest.h>
#include "common/Employee.h"
#include <cstring>

TEST(EmployeeTest, ValueAssignment) {
    Employee emp;
    emp.num = 100;
    emp.hours = 37.5;
   
    const char* employeeName = "Иван Иванов";
    strncpy_s(emp.name, employeeName, sizeof(emp.name) - 1);
    emp.name[sizeof(emp.name) - 1] = '\0';

    EXPECT_EQ(emp.num, 100);
    EXPECT_DOUBLE_EQ(emp.hours, 37.5);
    EXPECT_STREQ(emp.name, "Иван Иванов");
}



TEST(EmployeeTest, AssignmentOperator) {
    Employee emp1;
    emp1.num = 1;
    emp1.hours = 35.5;
    strncpy_s(emp1.name, "Первый сотрудник", sizeof(emp1.name) - 1);
    emp1.name[sizeof(emp1.name) - 1] = '\0';
    
    Employee emp2;
    emp2.num = 2;
    emp2.hours = 42.0;
    strncpy_s(emp2.name, "Второй сотрудник", sizeof(emp2.name) - 1);
    emp2.name[sizeof(emp2.name) - 1] = '\0';
    
    emp2 = emp1;
    
    EXPECT_EQ(emp2.num, 1);
    EXPECT_DOUBLE_EQ(emp2.hours, 35.5);
    EXPECT_STREQ(emp2.name, "Первый сотрудник");

    EXPECT_STREQ(emp1.name, "Первый сотрудник");
}

TEST(EmployeeTest, EdgeCases) {

    Employee emp1;
    emp1.num = -1;
    emp1.hours = -10.5;
    strncpy_s(emp1.name, "Negative", sizeof(emp1.name) - 1);
    
    EXPECT_EQ(emp1.num, -1);
    EXPECT_DOUBLE_EQ(emp1.hours, -10.5);
    
 
    Employee emp2;
    emp2.num = 0;
    emp2.hours = 0.0;
    emp2.name[0] = '\0';
    
    EXPECT_EQ(emp2.num, 0);
    EXPECT_DOUBLE_EQ(emp2.hours, 0.0);
    EXPECT_STREQ(emp2.name, "");
    
    
    Employee emp3;
    emp3.num = INT_MAX;
    emp3.hours = 999.99;
    
    EXPECT_EQ(emp3.num, INT_MAX);
    EXPECT_DOUBLE_EQ(emp3.hours, 999.99);
}


TEST(EmployeeTest, StringOperations) {
    Employee emp;
    

    strncpy_s(emp.name, "", sizeof(emp.name) - 1);
    emp.name[sizeof(emp.name) - 1] = '\0';
    EXPECT_STREQ(emp.name, "");
    EXPECT_EQ(strlen(emp.name), 0);
    
  
    strncpy_s(emp.name, "A", sizeof(emp.name) - 1);
    emp.name[sizeof(emp.name) - 1] = '\0';
    EXPECT_STREQ(emp.name, "A");
    EXPECT_EQ(strlen(emp.name), 1);
    
 
    std::string maxLengthStr(sizeof(emp.name) - 1, 'X');
    strncpy_s(emp.name, maxLengthStr.c_str(), sizeof(emp.name) - 1);
    emp.name[sizeof(emp.name) - 1] = '\0';
    EXPECT_EQ(strlen(emp.name), sizeof(emp.name) - 1);
}


TEST(EmployeeTest, BinaryCompatibility) {
    Employee emp;
    emp.num = 123;
    emp.hours = 45.67;
    strncpy_s(emp.name, "Test Employee", sizeof(emp.name) - 1);
    emp.name[sizeof(emp.name) - 1] = '\0';
    
    Employee empCopy;
    memcpy(&empCopy, &emp, sizeof(Employee));
    
    EXPECT_EQ(empCopy.num, emp.num);
    EXPECT_DOUBLE_EQ(empCopy.hours, emp.hours);
    EXPECT_STREQ(empCopy.name, emp.name);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    
    return RUN_ALL_TESTS();
}