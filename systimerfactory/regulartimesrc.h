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
#ifndef __REGULARTIMESRC_h_
#define __REGULARTIMESRC_h_

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "itimesrc.h"
#include <fcntl.h>


using namespace std;

class RegularTimeSrc : public ITimeSrc
{
	private:
		string m_path;
	public:
		RegularTimeSrc(string path):ITimeSrc(),m_path(std::move(path)){}
		bool isreference() { return false;}
		long long getTimeSec(){
			long long ret = 0;
			int fd = open(m_path.c_str(), O_RDWR | O_CLOEXEC, 0644);
                        if (fd < 0) {
                            // Handle error case (you may want to log this or throw an exception)
                            return ret; // Return early with the default value of ret
                        }

                        struct stat st;
                        if (fstat(fd, &st) >= 0) {
                           ret = st.st_mtim.tv_sec;
                        }

                        close(fd);
                        return ret;
                }
};
#endif// __REGULARTIMESRC_h_
