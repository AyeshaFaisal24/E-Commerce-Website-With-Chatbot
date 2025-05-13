// backend.cpp
// C++ Crow-based API for ChatBook online bookstore

#include <crow.h>
#include <sqlite_modern_cpp.h>
#include <optional>
#include <string>
#include <vector>

// === OOP Model Classes ===

class Book {
public:
    enum class Category { Fiction = 0, Religious = 1, Academic = 2 };

    Book(int id, const std::string& title, double price,
         Category category, const std::string& imageUrl)
        : id_(id), title_(title), price_(price),
          category_(category), imageUrl_(imageUrl) {}

    int getId() const { return id_; }
    const std::string& getTitle() const { return title_; }
    double getPrice() const { return price_; }
    Category getCategory() const { return category_; }
    const std::string& getImageUrl() const { return imageUrl_; }

private:
	
    int         id_;
    std::string title_;
    double      price_;
    Category    category_;
    std::string imageUrl_;
};

class User {
public:
    User(int id, const std::string& username, const std::string& passwordHash)
        : id_(id), username_(username), passwordHash_(passwordHash) {}

    int getId() const { return id_; }
    const std::string& getUsername() const { return username_; }
    bool verifyPassword(const std::string& candidate) const {
        return candidate == passwordHash_; // plaintext demo only
    }

private:
    int         id_;
    std::string username_;
    std::string passwordHash_;
};

class CartItem {
public:
    CartItem(const Book& book, int qty = 1)
        : book_(book), quantity_(qty) {}

    const Book& getBook() const { return book_; }
    int getQuantity() const { return quantity_; }
    void increment() { ++quantity_; }

private:
    Book  book_;
    int   quantity_;
};

// === Services ===

class AuthService {
public:
    explicit AuthService(sqlite::database& db) : db_(db) {}

    // Sign up new user
    crow::json::wvalue signup(const crow::json::rvalue& body) {
        std::string u = body["username"].s();
        std::string p = body["password"].s();
        crow::json::wvalue resp;
        try {
            db_ << "INSERT INTO users(username,passwordHash) VALUES(?,?);"
                << u << p;
            resp["status"] = "ok";
        } catch (sqlite::exception& e) {
            resp["status"] = "error";
            resp["message"] = "Username already exists";
        }
        return resp;
    }

    // Login existing user
    crow::json::wvalue login(const crow::json::rvalue& body) {
        std::string u = body["username"].s();
        std::string p = body["password"].s();
        std::optional<User> user;
        db_ << "SELECT id, username, passwordHash FROM users WHERE username = ?;"
            << u
            >> [&](int id, std::string uname, std::string phash) {
                if (phash == p) user = User(id, uname, phash);
            };
        crow::json::wvalue resp;
        if (user) {
            // TODO: set session or JWT token here
            resp["status"] = "ok";
        } else {
            resp["status"] = "error";
        }
        return resp;
    }

private:
    sqlite::database& db_;
};

class ProductService {
public:
    explicit ProductService(sqlite::database& db) : db_(db) {}

    crow::json::wvalue listByCategory(Book::Category cat) {
        crow::json::wvalue resp;
        auto& arr = resp["books"];
        db_ << "SELECT id, title, price, imageUrl FROM books WHERE category = ?;"
            << static_cast<int>(cat)
            >> [&](int id, std::string title, double price, std::string image) {
                crow::json::wvalue b;
                b["id"] = id;
                b["title"] = title;
                b["price"] = price;
                b["imageUrl"] = image;
                arr.push_back(b);
            };
        return resp;
    }

private:
    sqlite::database& db_;
};

class CartService {
public:
    void add(int bookId, const Book& book) {
        for (auto& it : items_) {
            if (it.getBook().getId() == bookId) { it.increment(); return; }
        }
        items_.emplace_back(book);
    }

    crow::json::wvalue list() const {
        crow::json::wvalue resp;
        auto& arr = resp["cart"];
        for (auto& it : items_) {
            const Book& b = it.getBook();
            crow::json::wvalue c;
            c["id"] = b.getId();
            c["title"] = b.getTitle();
            c["price"] = b.getPrice();
            c["quantity"] = it.getQuantity();
            c["imageUrl"] = b.getImageUrl();
            arr.push_back(c);
        }
        return resp;
    }

    void clear() { items_.clear(); }

private:
    std::vector<CartItem> items_;
};

// === Main App ===
int main() {
    crow::SimpleApp app;
    sqlite::database db("bookstore.db");

    AuthService    auth(db);
    ProductService product(db);
    CartService    cart;

    // Sign up new user
CROW_ROUTE(app, "/api/signup").methods("POST"_method)
([&](const crow::request& req){
    auto body = crow::json::load(req.body);
    if (!body) return crow::response(400);
    std::string u = body["username"].s();
    std::string p = body["password"].s();
    try {
      db << "INSERT INTO users (username, passwordHash) VALUES (?,?);"
         << u << p;
      crow::json::wvalue resp;
      resp["status"] = "ok";
      return crow::response{resp};
    } catch(...) {
      crow::json::wvalue resp;
      resp["status"] = "error";
      resp["message"] = "username taken";
      return crow::response{400, resp};
    }
});

    // Login
    CROW_ROUTE(app, "/api/login").methods("POST"_method)
    ([&](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400);
        auto resp = auth.login(body);
        return crow::response(resp);
    });

    // Books by category
    CROW_ROUTE(app, "/api/books")
    ([&](const crow::request& req) {
        auto qs = crow::query_string(req.url);
        int cat = qs.get_int("category");
        auto category = static_cast<Book::Category>(cat);
        return product.listByCategory(category);
    });

    // Add to cart
    CROW_ROUTE(app, "/api/cart/add").methods("POST"_method)
    ([&](const crow::request& req) {
        auto body = crow::json::load(req.body);
        int id = body["bookId"].i();
        Book::Category cat = static_cast<Book::Category>(body["category"].i());
        auto booksJson = product.listByCategory(cat);
        for (auto& b : booksJson["books"]) {
            if (b["id"].i() == id) {
                Book bk(id,
                        b["title"].s(),
                        b["price"].d(),
                        cat,
                        b["imageUrl"].s());
                cart.add(id, bk);
                return crow::response(200, "added");
            }
        }
        return crow::response(404, "not found");
    });

    // List cart
    CROW_ROUTE(app, "/api/cart")([&]{ return cart.list(); });

    // Checkout
    CROW_ROUTE(app, "/api/checkout").methods("POST"_method)
    ([&](const crow::request&){ cart.clear(); return crow::response(200, "ok"); });

    app.port(8080).multithreaded().run();
    return 0;
}
