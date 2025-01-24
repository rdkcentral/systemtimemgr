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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#define JSONRPC_CPP_CLIENT_H_
#define JSONRPCCPP_CLIENT_H_
#define JSONRPC_CPP_HTTPCLIENT_H_
#define JSONRPC_CLIENT_V2 2
#include <jsonrpccpp/common/jsonparser.h>
//#include <jsonrpccpp/common/exception.h>
#include <jsoncpp/json/value.h>
#include <vector>
#include <map>
class TestMock{
    public:
        MOCK_METHOD((Json::Value), getReturnValue,() );
        MOCK_METHOD((Json::Value), getReturnValue,(const std::string &name, const Json::Value &parameter));
};

static TestMock *globalTestClient = nullptr;

namespace jsonrpc{
    class JsonRpcException: public std::exception
    {
        public:
            JsonRpcException(const std::string& message):message(message){};
            virtual ~JsonRpcException() noexcept override {};
            virtual const char* what() const noexcept override {
        return message.c_str();
    }

        private:
            std::string message;
    };

class HttpClient
{
  public:
        HttpClient(const std::string& url){};
        void AddHeader (const std::string& attr, const std::string& val){
            //std::cout << "AddHeader called with Args " << attr << " and " << val << std::endl;
        }
};
class Client
    {
        public:
            TestMock *localtest = nullptr;
            Client(HttpClient &test,int CLIENT){localtest = globalTestClient;};
            Json::Value CallMethod (const std::string &name, const Json::Value &parameter){
                //std::cout << "CallMethod called with Args " << name << " and " << parameter << std::endl;
                Json::Value value =NULL;
                //printf("This is a test log local ptr = %p global ptr = %p\n",localtest,globalTestClient);
                if(localtest!=nullptr)
                    value = localtest->getReturnValue(name, parameter);
                    //std::cout << "Value from getReturnValue is " << value << std::endl;
                else
                    throw JsonRpcException("Client not initialized");
                    //std::cout << "Client not initialized" << std::endl;
                return value;
            }

    };


}
