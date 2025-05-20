// backend.cpp
// C++ Crow-based RESTful API for ChatBook Online Bookstore
// Built from scratch to simulate a minimal e-commerce backend with user authentication,
// product listing, and shopping cart functionality using OOP and SQLite

#include <crow.h>                  // Crow is a C++ microframework similar to Express.js
#include <sqlite_modern_cpp.h>     
#include <optional>
#include <string>
#include <vector>

// === Object-Oriented Model Classes ===

// Book class: Represents a book entity with metadata
class Book {
private:
    int id_;                    // Unique identifier for the book
    std::string title_;         // Title of the book
    double price_;              // Price in decimal format
    Category category_;         // Book category (enum)
    std::string imageUrl_;      // Link to cover image (frontend uses this)

public:
    // Enum for book categories to keep things type-safe and readable
    enum class Category { Fiction = 0, Religious = 1, Academic = 2 };

    // Constructor for initializing book properties
    Book(int id, const std::string& title, double price,
         Category category, const std::string& imageUrl)
        : id_(id), title_(title), price_(price),
          category_(category), imageUrl_(imageUrl) {}

    // Getters for accessing private attributes (encapsulation)
    int getId() const { return id_; }
    const std::string& getTitle() const { return title_; }
    double getPrice() const { return price_; }
    Category getCategory() const { return category_; }
    const std::string& getImageUrl() const { return imageUrl_; }
};

// User class: Represents a registered user
class User {
private:
    int id_;
    std::string username_;
    std::string passwordHash_;  

public:
    // Constructor initializes a user
    User(int id, const std::string& username, const std::string& passwordHash)
        : id_(id), username_(username), passwordHash_(passwordHash) {}

    int getId() const { return id_; }
    const std::string& getUsername() const { return username_; }

    // Verifies password 
    bool verifyPassword(const std::string& candidate) const {
        return candidate == passwordHash_; 
    }
};

// CartItem class: One line item in the cart
class CartItem {
private:
    Book book_;      // Book details (composition)
    int quantity_;   // Quantity of this book in the cart

public:
    // Initializes the cart item with one book
    CartItem(const Book& book, int qty = 1)
        : book_(book), quantity_(qty) {}

    const Book& getBook() const { return book_; }
    int getQuantity() const { return quantity_; }

    // Increments quantity if item already exists in cart
    void increment() { ++quantity_; }
};

// === Services ===

// Handles signup/login logic with the SQLite database
class AuthService {
private:
    sqlite::database& db_;
    
public:
    explicit AuthService(sqlite::database& db) : db_(db) {}

    // User registration logic
    crow::json::wvalue signup(const crow::json::rvalue& body) {
        std::string u = body["username"].s();
        std::string p = body["password"].s();
        crow::json::wvalue resp;

        try {
            db_ << "INSERT INTO users(username,passwordHash) VALUES(?,?);" << u << p;
            resp["status"] = "ok";
        } catch (sqlite::exception& e) {
            resp["status"] = "error";
            resp["message"] = "Username already exists";
        }
        return resp;
    }

    // User login logic
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
            resp["status"] = "ok"; // In real-world, return JWT or session token
        } else {
            resp["status"] = "error";
        }
        return resp;
    }
};

// Retrieves book listings from DB    
class ProductService {
private:
    sqlite::database& db_;
    
public:
    explicit ProductService(sqlite::database& db) : db_(db) {}

    // Return all books that match a given category
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
};

// Manages shopping cart state (stored in memory)
class CartService {
private:
    std::vector<CartItem> items_;  // Internal storage for cart items
    
public:
    // Adds item or increments if exists
    void add(int bookId, const Book& book) {
        for (auto& it : items_) {
            if (it.getBook().getId() == bookId) {
                it.increment();
                return;
            }
        }
        items_.emplace_back(book);
    }

    // Lists all cart items in JSON format
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

    // Empties the cart
    void clear() { items_.clear(); }

};

// === Main Web Application (Entry Point) ===
int main() {
    crow::SimpleApp app;            // Create the Crow application
    sqlite::database db("bookstore.db");  // Connect to SQLite DB

    // Instantiate services
    AuthService auth(db);
    ProductService product(db);
    CartService cart;

    // API Endpoint: Signup
    CROW_ROUTE(app, "/api/signup").methods("POST"_method)
    ([&](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400);

        std::string u = body["username"].s();
        std::string p = body["password"].s();

        try {
            db << "INSERT INTO users (username, passwordHash) VALUES (?,?);" << u << p;
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

    // API Endpoint: Login
    CROW_ROUTE(app, "/api/login").methods("POST"_method)
    ([&](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400);
        auto resp = auth.login(body);
        return crow::response(resp);
    });

    // API Endpoint: Get books by category
    CROW_ROUTE(app, "/api/books")
    ([&](const crow::request& req) {
        auto qs = crow::query_string(req.url);
        int cat = qs.get_int("category");
        auto category = static_cast<Book::Category>(cat);
        return product.listByCategory(category);
    });

    // API Endpoint: Add book to cart
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

    // API Endpoint: List all items in cart
    CROW_ROUTE(app, "/api/cart")([&]{ return cart.list(); });

    // API Endpoint: Checkout (clear cart)
    CROW_ROUTE(app, "/api/checkout").methods("POST"_method)
    ([&](const crow::request&) {
        cart.clear();
        return crow::response(200, "ok");
    });

    // Launch server on port 8080 with multithreading enabled
    app.port(8080).multithreaded().run();
    return 0;
}
