#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
using tcp = asio::ip::tcp;

class WebServer {
public:
    WebServer(asio::io_context& io_context, int port) 
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), root_directory_("public")
    {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket), root_directory_)->start();
                }
                do_accept();
            }
        );
    }

    tcp::acceptor acceptor_;
    std::string root_directory_;
};

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, const std::string& root_directory) 
        : socket_(std::move(socket)), root_directory_(root_directory) {}

    void start() {
        read_request();
    }

private:
    void read_request() {
        beast::http::async_read(
            socket_,
            buffer_,
            request_,
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (!ec) {
                    self->process_request();
                }
            }
        );
    }

    void process_request() {
        std::string target = request_.target().to_string();
        
        if (target == "/" || target == "/index.html") {
            serve_file("/index.html");
        } else if (target == "/style.css") {
            serve_file("/style.css", "text/css");
        } else {
            not_found();
        }
    }

    void serve_file(const std::string& path, const std::string& content_type = "text/html") {
        std::ifstream file(root_directory_ + path, std::ios::binary);
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            
            beast::http::response<beast::http::string_body> response{ beast::http::status::ok, request_.version() };
            response.set(beast::http::field::content_type, content_type);
            response.body() = content;
            response.prepare_payload();
            
            write_response(response);
        } else {
            not_found();
        }
    }

    void not_found() {
        beast::http::response<beast::http::string_body> response{ beast::http::status::not_found, request_.version() };
        response.set(beast::http::field::content_type, "text/plain");
        response.body() = "Not Found";
        response.prepare_payload();
        
        write_response(response);
    }

    void write_response(beast::http::response<beast::http::string_body>& response) {
        beast::http::async_write(
            socket_,
            response,
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            }
        );
    }

    tcp::socket socket_;
    beast::flat_buffer buffer_;
    beast::http::request<beast::http::string_body> request_;
    std::string root_directory_;
};

int main() {
    try {
        asio::io_context io_context;
        WebServer server(io_context, 8080);
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}