//
// Copyright (C) 2005 Mikkel Schubert (Xaignar@amule.org)
// Copyright (C) 2004 Barthelemy Dagenais (barthelemy@prologique.com)
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//


#include "testcase.h"
#include "test.h"
#include "testresult.h"

#include "../../src/MuleDebug.h"

#include <exception>

using namespace muleunit;

TestCase::TestCase(const wxString& name, TestResult *testResult)
		: m_failuresCount(0),
		  m_successesCount(0),
		  m_name(name),
		  m_testResult(testResult),
		  m_ran(false)
{}

TestCase::~TestCase()
{
}

void TestCase::addTest(Test *test)
{
	m_tests.push_back(test);
}

const TestList& TestCase::getTests() const
{
	return m_tests;
}

int TestCase::getTestsCount() const
{
	return m_tests.size();
}

int TestCase::getFailuresCount() const
{
	return m_failuresCount;
}

int TestCase::getSuccessesCount() const
{
	return m_successesCount;
}

bool TestCase::ran() const
{
	return m_ran;
}

const wxString& TestCase::getName() const
{
	return m_name;
}

void TestCase::updateCount(Test *test)
{
	if (test->failed()) {
		m_failuresCount++;
	} else {
		m_successesCount++;
	}
}

void TestCase::run()
{
	TestList::iterator it = m_tests.begin();
	for (; it != m_tests.end(); ++it) {
		Test* test = *it;

		test->setUp();

		try {
			test->run();
		} catch (const std::exception &e) {
			test->addTestPartResult(new TestPartResult(wxT(""), -1, wxConvLibc.cMB2WX(e.what()), error));
		} catch (const CInvalidArgsException& e) {
			test->addTestPartResult(new TestPartResult(wxT(""), -1, wxT("CInvalidArgsException: " ) + e.what(), error));			
		} catch (const CInvalidStateException& e) {
			test->addTestPartResult(new TestPartResult(wxT(""), -1, wxT("CInvalidStateException: " ) + e.what(), error));			
		} catch (const CBaseException& e) {
			test->addTestPartResult(new TestPartResult(wxT(""), -1, wxT("CBaseException: " ) + e.what(), error));			
		} catch (...) {
			test->addTestPartResult(new TestPartResult(wxT(""), -1, wxT("Unexpected exception occured"),error));
		}
		
		test->tearDown();
		updateCount(test);
	}

	m_ran = true;

	m_testResult->addResult(this);
}


