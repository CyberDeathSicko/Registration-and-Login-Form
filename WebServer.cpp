#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <bcrypt/BCrypt.hpp>
#include <random>
#include <unordered_map>

using namespace std;
namespace asio = boost::asio;
namespace beast = boost::beast;
using tcp = asio::ip::tcp;

struct User {
    string username;
    string hashedPassword;
};

vector<User> users = {
    {"user1", BCrypt::generateHash("password1")},
    {"user2", BCrypt::generateHash("password2")}
};

unordered_map<string, string> sessions;

bool isSessionValid(const string& sessionId) {
    return sessions.find(sessionId) != sessions.end();
}

bool verifyCredentials(const string& username, const string& password) {
    for (const User& user : users) {
        if (user.username == username && BCrypt::validatePassword(password, user.hashedPassword)) {
            return true;
        }
    }
    return false;
}

const string loginPage = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Login</title>
</head>
<body>
    <h1>Login</h1>
    <form action="/login" method="post">
        <label for="username">Username:</label>
        <input type="text" id="username" name="username" required><br><br>
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" required><br><br>
        <input type="submit" value="Login">
    </form>
</body>
</html>
)";

const string successPage = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Login Successful</title>
</head>
<body>
    <h1>Login Successful</h1>
    <p>Welcome, <span id="username">USERNAME</span>!</p>
</body>
</html>
)";

class RequestHandler {
public:
    RequestHandler(asio::io_context& io_context)
        : io_context_(io_context), socket_(io_context) {}

    void handleRequest() {
        beast::http::async_read(
            socket_,
            buffer_,
            request_,
            [this, self = shared_from_this()](beast::error_code ec, size_t) {
                if (!ec) {
                    processRequest();
                }
            }
        );
    }

private:
    void processRequest() {
        if (request_.method() == beast::http::verb::post && request_.target() == "/login") {
            handleLogin();
        } else if (request_.method() == beast::http::verb::get && request_.target() == "/login") {
            sendHtmlResponse(loginPage);
        } else if (request_.method() == beast::http::verb::get && request_.target() == "/success") {
            handleSuccess();
        } else {
            sendResponse(beast::http::status::not_found, "Not Found");
        }
    }

    void handleLogin() {
        string username = request_["username"].to_string();
        string password = request_["password"].to_string();

        if (verifyCredentials(username, password)) {
            string sessionId = generateSessionId();
            sessions[sessionId] = username;
            string successPageWithUsername = successPage;
            size_t pos = successPageWithUsername.find("USERNAME");
            if (pos != string::npos) {
                successPageWithUsername.replace(pos, strlen("USERNAME"), username);
            }
            sendHtmlResponse(successPageWithUsername);
        } else {
            string errorMessage = "Invalid username or password.";
            sendRedirect("/login?error=" + errorMessage);
        }
    }

    void handleSuccess() {
        string sessionId = request_["Cookie"];
        if (isSessionValid(sessionId)) {
            string username = sessions[sessionId];
            string successPageWithUsername = successPage;
            size_t pos = successPageWithUsername.find("USERNAME");
            if (pos != string::npos) {
                successPageWithUsername.replace(pos, strlen("USERNAME"), username);
            }
            sendHtmlResponse(successPageWithUsername);
        } else {
            sendRedirect("/login");
        }
    }

    void sendHtmlResponse(const string& content) {
        beast::http::response<beast::http::string_body> response(beast::http::status::ok, request_.version());
        response.set(beast::http::field::content_type, "text/html");
        response.body() = content;
        response.prepare_payload();
        sendResponse(response);
    }

    void sendRedirect(const string& location) {
        beast::http::response<beast::http::empty_body> response(beast::http::status::see_other, request_.version());
        response.set(beast::http::field::location, location);
        response.prepare_payload();
        sendResponse(response);
    }

    void sendResponse(beast::http::response<beast::http::string_body>& response) {
        beast::http::async_write(
            socket_,
            response,
            [this, self = shared_from_this()](beast::error_code ec, size_t) {
                socket_.shutdown(tcp::socket::shutdown_send, ec);
                if (!ec) {
                    beast::http::async_read(socket_, buffer_, request_, beast::bind_front_handler(&RequestHandler::handleRequest, shared_from_this()));
                }
            }
        );
    }

    string generateSessionId() {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<int> dist(0, 15);
        const string chars = "0123456789abcdef";
        string sessionId = "";
        for (int i = 0; i < 32; ++i) {
            sessionId += chars[dist(gen)];
        }
        return sessionId;
    }

    asio::io_context& io_context_;
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    beast::http::request<beast::http::string_body> request_;
};

int main() {
    try {
        asio::io_context io_context;
        WebServer server(io_context, 8080);
        io_context.run();
    } catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
    }

    return 0;
}