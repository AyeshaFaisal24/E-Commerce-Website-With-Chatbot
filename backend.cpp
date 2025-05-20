#include <crow.h>                    //Crow is a C++ microframework for building web APIs (similar to Express.js in Node).
#include <sqlite_modern_cpp.h>      // This is a modern C++ wrapper for SQLite database.
#include <optional>                // Helpful in database queries when results may be empty (e.g., user not found).
#include <string>
#include <vector>
using namespace std;

// === OOP Model Classes ===

class Book {
    private:
        int id_;
        string title_;
        double price_;
        Category category_;
        string imageUrl_;

    public:
        enum class Category { Fiction = 0, Religious = 1, Academic = 2 };

        Book(int id, string title, double price, Category category, string imageUrl)
            : id_(id), title_(title), price_(price),category_(category), imageUrl_(imageUrl) {}

        int getId() const { return id_; }
        string getTitle() const { return title_; }
        double getPrice() const { return price_; }
        Category getCategory() const { return category_; }
        string getImageUrl() const { return imageUrl_; }

};

class User {
    private:
        int id_;
        string username_;
        string passwordHash_;

    public:
        User(int id,string username,string passwordHash)
            : id_(id), username_(username), passwordHash_(passwordHash) {}

        int getId() const { return id_; }
        string getUsername() const { return username_; }
        bool verifyPassword(string candidate) const {                    //candidate is the password which user enters
            return candidate == passwordHash_;                 // plaintext demo only
        }

};

class CartItem {
    private:
        Book book_;
        int quantity_;  

    public:
        CartItem(Book book, int qty = 1) : book_(book), quantity_(qty) {}

        Book& getBook() const { return book_; }
        int getQuantity() const { return quantity_; }
        void increment() { ++quantity_; }

};

// === Services ===

class AuthService {
    private:
        sqlite::database& db_;
    
    public:
        explicit AuthService(sqlite::database& db) : db_(db) {}

        // Sign up new user
        crow::json::wvalue signup(const crow::json::rvalue& body) {  //body — JSON data from the frontend, containing username and password
            string u = body["username"].s();      //Extracts "username" string from the JSON request body.
            string p = body["password"].s();
            crow::json::wvalue resp;                  // Create JSON response object to send back to client

            try {
                db_ << "INSERT INTO users(username,passwordHash) VALUES(?,?);"   //Inserts the new user into the users table with the username and password.
                    << u << p;
                resp["status"] = "ok";            	//Mark signup as successful
            } catch (sqlite::exception& e) {
                resp["status"] = "error";         //Mark signup as failed
                resp["message"] = "Username already exists";
            }
            return resp;
        }

        // Login existing user
        crow::json::wvalue login(const crow::json::rvalue& body) {
            string u = body["username"].s();
            string p = body["password"].s();
            optional<User> user;           //Creates a variable that might store a user object
            db_ << "SELECT id, username, passwordHash FROM users WHERE username = ?;"      //It asks the database(username etc.)
                << u
                >> [&](int id, string uname,string phash) {
                    if (phash == p) user = User(id, uname, phash); 
                };
            crow::json::wvalue resp;
            if (user) {
                resp["status"] = "ok";
            } else {
                resp["status"] = "error";
            }
            return resp;
        }

};

class ProductService {
    private:
        sqlite::database& db_;   //class will use this database connection to run queries

    public:
        explicit ProductService(sqlite::database& db) : db_(db) {}

        crow::json::wvalue listByCategory(Book::Category cat) {
            crow::json::wvalue resp;    //Creates an empty JSON object called resp
            auto& arr = resp["books"];        //In this arr book details will be added
            db_ << "SELECT id, title, price, imageUrl FROM books WHERE category = ?;"    //selects that category from the table which mathces the input
                << static_cast<int>(cat)      //converts the category enum to an integer (because database stores category as int).
                >> [&](int id, std::string title, double price, std::string image) {
                    crow::json::wvalue b;   //Inside the lambda, a new JSON object b is created
                    b["id"] = id;
                    b["title"] = title;
                    b["price"] = price;
                    b["imageUrl"] = image;
                    arr.push_back(b);           ////Book details from the database (id, title, price, image URL) are added as key-value pairs in b
                };
            return resp;  //returns the JSON response object which contains all books in the given category
        }
};

class CartService {
    private:
        vector<CartItem> items_;       //list of all books added to the cart

    public:
        void add(int bookId, const Book& book) {
            for (auto& it : items_) {
                if (it.getBook().getId() == bookId) {    // Book already exists, so just increase quantity
                    it.increment(); 
                    return; 
                }
            }
            items_.emplace_back(book);   // Book not in cart yet, so add it
        }

        crow::json::wvalue list() const {           // Returns the cart as a JSON object (for frontend)
            crow::json::wvalue resp;           // Create JSON obj for response
            auto& arr = resp["cart"];          // Make an array called "cart" inside obj 

            for (auto& it : items_) {         //Loop through each item in the cart
                const Book& b = it.getBook();    // Get the book from the cart item  
                crow::json::wvalue c;            // Create a small JSON object named c for one cart item
                c["id"] = b.getId();
                c["title"] = b.getTitle();
                c["price"] = b.getPrice();
                c["quantity"] = it.getQuantity();
                c["imageUrl"] = b.getImageUrl();
                arr.push_back(c);       //This adds the item c (one book’s info) to the "cart" array in the main JSON object
            }
            return resp;      //Once all items are added, return the full JSON object resp
        }

        void clear() {        // Empties the cart
            items_.clear(); 
        }

};

// === Main App ===
int main() {
    crow::SimpleApp app;        //Creates a simple Crow web server application called app
    sqlite::database db("bookstore.db");         //opens SQLite database file

    AuthService    auth(db);       //auth handles login/signup
    ProductService product(db);    //product handles books/products
    CartService    cart;           //cart handles cart operations

    // Sign up new user
    CROW_ROUTE(app, "/api/signup").methods("POST"_method)      //Defines an HTTP POST route: /api/signup
    ([&](const crow::request& req){
    auto body = crow::json::load(req.body);          
    if (!body) return crow::response(400);           //Reads JSON data from the request body. If it's invalid, return a 400 Bad Request
    std::string u = body["username"].s();            //Extracts the username and password from the JSON body
    std::string p = body["password"].s();

    try {            //Tries inserting the user into the database
        db << "INSERT INTO users (username, passwordHash) VALUES (?,?);"
            << u << p;
        crow::json::wvalue resp;
        resp["status"] = "ok";
        return crow::response{resp};
    }
    catch(...) {
        crow::json::wvalue resp;
        resp["status"] = "error";
        resp["message"] = "username taken";
        return crow::response{400, resp};
    }
    }
    );

    // Login
    CROW_ROUTE(app, "/api/login").methods("POST"_method)        //Route for user login
    ([&](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400);
        auto resp = auth.login(body);           //Calls the login() method from AuthService and returns its response
        return crow::response(resp);
    });

    // Books by category
    CROW_ROUTE(app, "/api/books")        //HTTP GET route for /api/book
    ([&](const crow::request& req) {
        auto qs = crow::query_string(req.url);
        int cat = qs.get_int("category");
        auto category = static_cast<Book::Category>(cat);    //Gets the category number and converts it to an enum type
        return product.listByCategory(category);
    });

    // Add to cart
    CROW_ROUTE(app, "/api/cart/add").methods("POST"_method)          //POST route to add a book to the cart
    ([&](const crow::request& req) {
        auto body = crow::json::load(req.body);
        int id = body["bookId"].i();              //Reads book ID from request body.
        Book::Category cat = static_cast<Book::Category>(body["category"].i());
        auto booksJson = product.listByCategory(cat);         //Gets the full list of books in that category

        for (auto& b : booksJson["books"]) {   //Loops through books to find the matching book by ID
            if (b["id"].i() == id) {
                Book bk(id,             //Creates a Book object with the found data
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

    //Route for viewing cart items. Returns a list of books in the cart
    CROW_ROUTE(app, "/api/cart")([&]{ return cart.list(); });

    // Checkout
    CROW_ROUTE(app, "/api/checkout").methods("POST"_method)
    ([&](const crow::request&){ cart.clear(); return crow::response(200, "ok"); });

    app.port(8080).multithreaded().run();       //Starts the server on port 8080 using multiple threads.
    return 0;
}
