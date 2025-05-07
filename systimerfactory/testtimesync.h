/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _TESTTIMESYNC_H_
#define _TESTTIMESYNC_H_

#include <unistd.h>
#include "itimesrc.h"
#include <fcntl.h>
#include <fstream>
#include "irdklog.h"

using namespace std;
class TestTimeSync: public ITimeSync
{
	private:
		string m_path;
	public: 
		TestTimeSync(string path):ITimeSync(),m_path(std::move(path)) {}
		~TestTimeSync(){}
		void  updateTime(long long locTime) {
			RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Updating time \n",__FUNCTION__,__LINE__);
			ofstream timefile(m_path.c_str(),ios::out);
			if (timefile.is_open())
			{
				timefile<<locTime<<"\n";
				timefile.close();
			}
		}

		long long getTime() {
			long long ret = 0;
			fstream myfile(m_path.c_str(), std::ios_base::in);
			myfile>>ret;
			RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Time Returning = %lld \n",__FUNCTION__,__LINE__,ret);
			return ret;
		}
};

#endif// _TESTTIMESYNC_H_
