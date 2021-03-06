/*
The MIT License (MIT)

Copyright (c) 2013 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <srs_core.hpp>

#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <srs_core_log.hpp>

static int64_t _srs_system_time_us_cache = 0;

int64_t srs_get_system_time_ms()
{
	return _srs_system_time_us_cache / 1000;
}

void srs_update_system_time_ms()
{
    timeval now;
    
    gettimeofday(&now, NULL);

    // we must convert the tv_sec/tv_usec to int64_t.
    _srs_system_time_us_cache = ((int64_t)now.tv_sec) * 1000 * 1000 + (int64_t)now.tv_usec;
    
    _srs_system_time_us_cache = srs_max(0, _srs_system_time_us_cache);
}

std::string srs_replace(std::string str, std::string old_str, std::string new_str)
{
	std::string ret = str;
	
	if (old_str == new_str) {
		return ret;
	}
	
	size_t pos = 0;
	while ((pos = ret.find(old_str, pos)) != std::string::npos) {
		ret = ret.replace(pos, old_str.length(), new_str);
		pos += new_str.length();
	}
	
	return ret;
}

std::string srs_dns_resolve(std::string host)
{
    if (inet_addr(host.c_str()) != INADDR_NONE) {
        return host;
    }
    
    hostent* answer = gethostbyname(host.c_str());
    if (answer == NULL) {
        srs_error("dns resolve host %s error.", host.c_str());
        return "";
    }
    
    char ipv4[16];
    memset(ipv4, 0, sizeof(ipv4));
    for (int i = 0; i < answer->h_length; i++) {
        inet_ntop(AF_INET, answer->h_addr_list[i], ipv4, sizeof(ipv4));
        srs_info("dns resolve host %s to %s.", host.c_str(), ipv4);
        break;
    }
    
    return ipv4;
}

void srs_vhost_resolve(std::string& vhost, std::string& app)
{
	app = srs_replace(app, "...", "?");
	
	size_t pos = 0;
	if ((pos = app.find("?")) == std::string::npos) {
		return;
	}
	
	std::string query = app.substr(pos + 1);
	app = app.substr(0, pos);
	
	if ((pos = query.find("vhost?")) != std::string::npos
		|| (pos = query.find("vhost=")) != std::string::npos
		|| (pos = query.find("Vhost?")) != std::string::npos
		|| (pos = query.find("Vhost=")) != std::string::npos
	) {
		query = query.substr(pos + 6);
		if (!query.empty()) {
			vhost = query;
		}
	}
}

void srs_close_stfd(st_netfd_t& stfd)
{
	if (stfd) {
		int fd = st_netfd_fileno(stfd);
		st_netfd_close(stfd);
		stfd = NULL;
		
		// st does not close it sometimes, 
		// close it manually.
		close(fd);
	}
}

