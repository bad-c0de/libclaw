# libclaw
libclaw is a web requests library written in C that can send http/https requests and recieve responses.
DESCLAIMER: This is my first library ever so I'm sorry for any bug that you encounter.

# Installation
Clone the Repo:
```
git clone https://github.com/bad-c0de/libclaw.git
```

Run the make file:
```
sudo make install
```

# Usage
code example for get requests:
```C
#include <libclaw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    req_t req;
    // WARNING: URL MUST END WITH '/'
    char* url = "http://127.0.0.1:8080/";
    char* headers = "Host: 127.0.0.1\r\n"
                    "User-Agent: requests\r\n"
                    "Accept: */*\r\n\r\n";

    init_request(&req, url);
    get_request(&req, url, headers); // You can repalce headers with NULL to use the default headers

    // print status code and the body of the website
    printf("status code: %d\n\n", req.status_code);
    printf("%s\n", req.text);
    return 0;
}
```

Compile:
```
gcc -o <program> <program.c> -lclaw -lssl -lcrypto
```
