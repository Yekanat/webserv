# webserv — A tiny yet capable HTTP/1.1 server in C++98

![C++98](https://img.shields.io/badge/C%2B%2B-98-blue)
![HTTP/1.1](https://img.shields.io/badge/HTTP-1.1-2ea44f)
![Methods](https://img.shields.io/badge/Methods-GET%20%7C%20POST%20%7C%20DELETE-0366d6)
![CGI](https://img.shields.io/badge/CGI-supported-ff9800)
![Uploads](https://img.shields.io/badge/File%20Uploads-supported-9c27b0)
![Config](https://img.shields.io/badge/Config-NGINX--style-6f42c1)

A compact, educational web server that implements the essentials of HTTP/1.1 in portable C++98. It serves static files, runs CGI scripts, supports file uploads, and uses an easy-to-read NGINX-style configuration.

Whether you’re exploring how web servers work under the hood or need a lightweight server with minimal dependencies, webserv is designed to be clear, hackable, and practical.

- Repository: [Yekanat/webserv](https://github.com/Yekanat/webserv)
- Description: HTTP/1.1 server in C++98 • GET/POST/DELETE • CGI & file upload • NGINX-style config

---

## Highlights

- HTTP/1.1 core features: persistent connections, request parsing, and basic routing
- Methods: GET, POST, DELETE
- Static file serving with optional directory listings
- CGI support (e.g., run Python/Perl/Bash scripts via the CGI interface)
- File uploads (multipart/form-data)
- NGINX-style configuration with server and location blocks
- Custom error pages and per-location method restrictions
- Single binary, minimal runtime dependencies

Note: Exact directive names and feature flags depend on this project’s implementation; see the examples and any sample configs in the repository.

---

## Quick start

### Prerequisites
- A C++98-compatible compiler (e.g., GCC or Clang)
- A POSIX-like environment (Linux/macOS/WSL)
- make (or your build toolchain)
- Optional for CGI: Python/Perl/Bash interpreters installed

### Build
If the repository provides a Makefile:
```
make
```

Otherwise, a typical manual compile command could look like:
```
g++ -std=c++98 -Wall -Wextra -Werror -O2 -I include -o webserv \
    $(find src -name '*.cpp')
```
Adjust include/source paths according to the repo structure.

### Run
```
./webserv path/to/config.conf
```
- If the project includes a sample config (often in configs/ or the repo root), try it first:
```
./webserv configs/webserv.conf
```

---

## Configuration (NGINX-style)

Webserv uses an NGINX-style configuration syntax with server and location blocks. Below is an example to help you get started. The exact directive names and supported options are defined by this project—refer to the provided sample configs for the authoritative reference.

```nginx
server {
    listen              0.0.0.0:8080;
    server_name         localhost;

    root                ./www;
    index               index.html;

    # Serve a custom 404 page
    error_page          404 /errors/404.html;

    # Limit request bodies (e.g., uploads)
    client_max_body_size 8m;

    # Default location
    location / {
        methods         GET;
        autoindex       on;         # Enable directory listing
    }

    # Handle file uploads
    location /upload {
        methods         POST;
        # Project-specific directive name may differ (example):
        upload_store    ./uploads;  # Where uploaded files are saved
    }

    # Allow deleting files (use carefully!)
    location /delete {
        methods         DELETE;
        root            ./www/deletable;
    }

    # Run CGI scripts in this path (example for Python)
    location /cgi {
        # Project-specific directive name may differ (examples):
        cgi_pass        /usr/bin/python3;   # Interpreter for *.py
        cgi_extension   .py;                # Treat *.py as CGI
        # Alternatively, some builds auto-detect by shebang (#!/usr/bin/env python3)
    }
}
```

Tips:
- Use server_name and listen to host multiple virtual servers on different ports or hostnames.
- Place custom error pages under a known path (e.g., ./www/errors/).
- Keep uploads in a dedicated directory and restrict methods per location.
- Consult the repository’s sample configs for the exact directives supported.

---

## Usage examples

Assuming the server is running on http://localhost:8080:

- GET a static file
```
curl http://localhost:8080/
curl http://localhost:8080/images/logo.png
```

- POST form data
```
curl -X POST -d "name=webserv&hello=world" http://localhost:8080/form
```

- Upload a file (multipart/form-data)
```
curl -F "file=@./path/to/photo.jpg" http://localhost:8080/upload
```

- DELETE a resource (be careful!)
```
curl -X DELETE http://localhost:8080/delete/old.txt
```

- Run a CGI script
```
curl http://localhost:8080/cgi/hello.py
```

---

## CGI quickstart

Place a script under the configured CGI location and ensure it’s executable with a valid shebang. For example, ./www/cgi/hello.py:

```python
#!/usr/bin/env python3
print("Status: 200 OK")
print("Content-Type: text/plain")
print()
print("Hello from CGI! ✨")
```

Make it executable:
```
chmod +x ./www/cgi/hello.py
```

Then:
```
curl http://localhost:8080/cgi/hello.py
```

---

## Error pages

Customize error pages with the error_page directive, e.g.:
```
error_page 404 /errors/404.html;
```

Create ./www/errors/404.html:
```html
<!doctype html>
<html lang="en">
  <head><meta charset="utf-8"><title>Not Found</title></head>
  <body><h1>404 — Not Found</h1><p>Sorry, we couldn’t find that.</p></body>
</html>
```

---

## Development notes

- Language composition
  - C++: 63.3%
  - Shell: 23.7%
  - HTML: 12.4%
  - Other: 0.6%

- Guidelines
  - Prefer portable C++98 constructs
  - Keep I/O, parsing, and response logic well-separated
  - Add small, focused CGI examples for testing

- Debugging tips
  - Start with a minimal config and a single location
  - Use curl -v to inspect requests/responses
  - Tail your server logs (if provided) while testing

---

## FAQ

- Does it support persistent connections?
  - Yes, it implements core HTTP/1.1 behavior. Exact keep-alive semantics follow the project’s parser/response logic.

- Which CGI interpreters are supported?
  - Any interpreter reachable on your system (e.g., Python, Perl, Bash) that can run via the configured CGI directives or via script shebang lines.

- How large can uploads be?
  - Controlled by client_max_body_size (see your config). Ensure the destination directory exists and has write permissions.

---

## Roadmap ideas

- More detailed logging and access/error logs
- MIME type mapping for richer static file responses
- Better directory listing themes
- TLS termination (via reverse proxy or native support)
- More robust configuration validation and error messages

---

## Contributing

Issues, ideas, and pull requests are welcome!
- Discuss new directives and semantics before large changes
- Keep contributions focused and well-tested
- Add/adjust sample configs and CGI examples where helpful

---

## License

This project is distributed under the license specified in the repository. See the LICENSE file for details.

---

## Acknowledgments

- Inspired by the clarity of NGINX’s configuration style
- Thanks to the authors of the HTTP/1.1 specs (RFC 2616 and its successors RFC 7230–7235)
- Everyone exploring systems programming and networking—have fun building!
