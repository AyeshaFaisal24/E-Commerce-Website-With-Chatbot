#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <stdexcept>
using namespace std;

// Abstract Base Class for Books
class Book {
protected:
    string title;
    string author;
    string ISBN;
    float price;
    int stock;
    string category;

public:
    Book(string t, string a, string isbn, float p, int s, string cat)
        : title(t), author(a), ISBN(isbn), price(p), stock(s), category(cat) {}

    // Pure virtual function making this an abstract class
    virtual string getDescription() = 0;

    // Getters
    string getTitle() const { return title; }
    string getAuthor() const { return author; }
    string getISBN() const { return ISBN; }
    float getPrice() const { return price; }
    int getStock() const { return stock; }
    string getCategory() const { return category; }

    // Setters
    void setPrice(float p) { price = p; }
    void setStock(int s) { stock = s; }

    // Decrease stock when purchased
    void purchase(int quantity) {
        if (stock >= quantity) {
            stock -= quantity;
        } else {
            throw runtime_error("Not enough stock available");
        }
    }

    virtual ~Book() {}
};

// Derived Classes for each Book Category
class AcademicBook : public Book {
public:
    AcademicBook(string t, string a, string isbn, float p, int s)
        : Book(t, a, isbn, p, s, "Academic") {}

    string getDescription() override {
        return title + " by " + author + " (Academic) - $" + to_string(price);
    }
};

class FictionBook : public Book {
public:
    FictionBook(string t, string a, string isbn, float p, int s)
        : Book(t, a, isbn, p, s, "Fiction") {}

    string getDescription() override {
        return title + " by " + author + " (Fiction) - $" + to_string(price);
    }
};

class ReligiousBook : public Book {
public:
    ReligiousBook(string t, string a, string isbn, float p, int s)
        : Book(t, a, isbn, p, s, "Religious") {}

    string getDescription() override {
        return title + " by " + author + " (Religious) - $" + to_string(price);
    }
};

// Shopping Cart Class
class ShoppingCart {
private:
    map<string, pair<shared_ptr<Book>, int>> items; // ISBN to (book, quantity)

public:
    void addItem(shared_ptr<Book> book, int quantity = 1) {
        if (items.find(book->getISBN()) != items.end()) {
            items[book->getISBN()].second += quantity;
        } else {
            items[book->getISBN()] = make_pair(book, quantity);
        }
    }

    void removeItem(string ISBN) {
        items.erase(ISBN);
    }

    void updateQuantity(string ISBN, int newQuantity) {
        if (items.find(ISBN) != items.end()) {
            items[ISBN].second = newQuantity;
        }
    }

    float getTotal() const {
        float total = 0.0f;
        for (const auto& item : items) {
            total += item.second.first->getPrice() * item.second.second;
        }
        return total;
    }

    void checkout() {
        for (auto& item : items) {
            item.second.first->purchase(item.second.second);
        }
        items.clear();
    }

    void displayCart() const {
        cout << "\nShopping Cart Contents:\n";
        cout << "-----------------------\n";
        for (const auto& item : items) {
            cout << item.second.first->getDescription() 
                 << " x" << item.second.second << endl;
        }
        cout << "-----------------------\n";
        cout << "Total: $" << getTotal() << "\n\n";
    }
};

// User Base Class
class User {
protected:
    string username;
    string password;
    string email;

public:
    User(string uname, string pwd, string em) 
        : username(uname), password(pwd), email(em) {}

    // Virtual function for polymorphism
    virtual string getRole() const = 0;

    // Getters
    string getUsername() const { return username; }
    string getEmail() const { return email; }

    bool authenticate(string uname, string pwd) const {
        return (username == uname && password == pwd);
    }

    virtual ~User() {}
};

// Customer Class
class Customer : public User {
private:
    ShoppingCart cart;

public:
    Customer(string uname, string pwd, string em)
        : User(uname, pwd, em) {}

    string getRole() const override { return "Customer"; }

    ShoppingCart& getCart() { return cart; }

    void viewCart() {
        cart.displayCart();
    }
};

// Admin Class
class Admin : public User {
public:
    Admin(string uname, string pwd, string em)
        : User(uname, pwd, em) {}

    string getRole() const override { return "Admin"; }

    void addBook(vector<shared_ptr<Book>>& inventory, shared_ptr<Book> book) {
        inventory.push_back(book);
    }

    void updateBookPrice(vector<shared_ptr<Book>>& inventory, string ISBN, float newPrice) {
        for (auto& book : inventory) {
            if (book->getISBN() == ISBN) {
                book->setPrice(newPrice);
                return;
            }
        }
        throw runtime_error("Book not found");
    }

    void restockBook(vector<shared_ptr<Book>>& inventory, string ISBN, int quantity) {
        for (auto& book : inventory) {
            if (book->getISBN() == ISBN) {
                book->setStock(book->getStock() + quantity);
                return;
            }
        }
        throw runtime_error("Book not found");
    }
};

// Bookstore Class (Singleton)
class Bookstore {
private:
    static Bookstore* instance;
    vector<shared_ptr<Book>> inventory;
    vector<shared_ptr<User>> users;

    Bookstore() {
        initializeInventory();
        initializeUsers();
    }

    void initializeInventory() {
        // Academic Books (12 books, 3 copies each)
        inventory.push_back(make_shared<AcademicBook>("Acids And Bases", "Dr. Smith", "9780123456789", 1596.0f, 3));
        inventory.push_back(make_shared<AcademicBook>("Equity In Science", "Dr. Johnson", "9780123456790", 1596.0f, 3));
        inventory.push_back(make_shared<AcademicBook>("International History", "Prof. Lee", "9780123456791", 1790.0f, 3));
        inventory.push_back(make_shared<AcademicBook>("World's Oceans", "Dr. Brown", "9780123456792", 1907.0f, 3));
        inventory.push_back(make_shared<AcademicBook>("Dictionary", "Oxford Press", "9780123456793", 1596.0f, 3));
        inventory.push_back(make_shared<AcademicBook>("Quantitative Finance", "Dr. Wilson", "9780123456794", 1790.0f, 3));
        inventory.push_back(make_shared<AcademicBook>("Standard Mathematics", "Prof. Davis", "9780123456795", 1596.0f, 3));
        inventory.push_back(make_shared<AcademicBook>("Essential Grammar", "Dr. Taylor", "9780123456796", 500.0f, 3));
        inventory.push_back(make_shared<AcademicBook>("Fundamentals Of Economics", "Prof. Clark", "9780123456797", 1596.0f, 3));
        inventory.push_back(make_shared<AcademicBook>("Astronomy Guide", "Dr. Adams", "9780123456798", 1656.0f, 3));
        inventory.push_back(make_shared<AcademicBook>("Artificial Intelligence Basics", "Prof. White", "9780123456799", 1596.0f, 3));
        inventory.push_back(make_shared<AcademicBook>("Cybersecurity", "Dr. Green", "9780123456800", 1907.0f, 3));

        // Fiction Books (12 books, 3 copies each)
        inventory.push_back(make_shared<FictionBook>("The Great Adventure", "John Doe", "9781123456789", 1200.0f, 3));
        inventory.push_back(make_shared<FictionBook>("Mystery of the Night", "Jane Smith", "9781123456790", 1500.0f, 3));
        inventory.push_back(make_shared<FictionBook>("Space Odyssey", "Arthur Clarke", "9781123456791", 1800.0f, 3));
        inventory.push_back(make_shared<FictionBook>("The Last Kingdom", "Bernard Cornwell", "9781123456792", 1300.0f, 3));
        inventory.push_back(make_shared<FictionBook>("1984", "George Orwell", "9781123456793", 1100.0f, 3));
        inventory.push_back(make_shared<FictionBook>("Pride and Prejudice", "Jane Austen", "9781123456794", 1000.0f, 3));
        inventory.push_back(make_shared<FictionBook>("The Hobbit", "J.R.R. Tolkien", "9781123456795", 1400.0f, 3));
        inventory.push_back(make_shared<FictionBook>("Dune", "Frank Herbert", "9781123456796", 1600.0f, 3));
        inventory.push_back(make_shared<FictionBook>("The Alchemist", "Paulo Coelho", "9781123456797", 900.0f, 3));
        inventory.push_back(make_shared<FictionBook>("The Da Vinci Code", "Dan Brown", "9781123456798", 1500.0f, 3));
        inventory.push_back(make_shared<FictionBook>("Harry Potter", "J.K. Rowling", "9781123456799", 1700.0f, 3));
        inventory.push_back(make_shared<FictionBook>("The Shining", "Stephen King", "9781123456800", 1300.0f, 3));

        // Religious Books (12 books, 3 copies each)
        inventory.push_back(make_shared<ReligiousBook>("The Holy Bible", "Various", "9782123456789", 2000.0f, 3));
        inventory.push_back(make_shared<ReligiousBook>("The Quran", "Various", "9782123456790", 1800.0f, 3));
        inventory.push_back(make_shared<ReligiousBook>("Bhagavad Gita", "Vyasa", "9782123456791", 1500.0f, 3));
        inventory.push_back(make_shared<ReligiousBook>("The Torah", "Various", "9782123456792", 1700.0f, 3));
        inventory.push_back(make_shared<ReligiousBook>("The Upanishads", "Various", "9782123456793", 1600.0f, 3));
        inventory.push_back(make_shared<ReligiousBook>("The Book of Mormon", "Joseph Smith", "9782123456794", 1400.0f, 3));
        inventory.push_back(make_shared<ReligiousBook>("Tao Te Ching", "Laozi", "9782123456795", 1200.0f, 3));
        inventory.push_back(make_shared<ReligiousBook>("The Art of Happiness", "Dalai Lama", "9782123456796", 1300.0f, 3));
        inventory.push_back(make_shared<ReligiousBook>("The Power of Now", "Eckhart Tolle", "9782123456797", 1100.0f, 3));
        inventory.push_back(make_shared<ReligiousBook>("The Purpose Driven Life", "Rick Warren", "9782123456798", 1000.0f, 3));
        inventory.push_back(make_shared<ReligiousBook>("Mere Christianity", "C.S. Lewis", "9782123456799", 900.0f, 3));
        inventory.push_back(make_shared<ReligiousBook>("The Case for Christ", "Lee Strobel", "9782123456800", 1500.0f, 3));
    }

    void initializeUsers() {
        users.push_back(make_shared<Customer>("customer1", "pass123", "customer1@example.com"));
        users.push_back(make_shared<Customer>("customer2", "pass456", "customer2@example.com"));
        users.push_back(make_shared<Admin>("admin", "admin123", "admin@example.com"));
    }

public:
    static Bookstore* getInstance() {
        if (!instance) {
            instance = new Bookstore();
        }
        return instance;
    }

    // Prevent copying
    Bookstore(const Bookstore&) = delete;
    void operator=(const Bookstore&) = delete;

    vector<shared_ptr<Book>> getInventory() const { return inventory; }

    vector<shared_ptr<Book>> getBooksByCategory(const string& category) {
        vector<shared_ptr<Book>> result;
        for (const auto& book : inventory) {
            if (book->getCategory() == category) {
                result.push_back(book);
            }
        }
        return result;
    }

    shared_ptr<User> authenticateUser(const string& username, const string& password) {
        for (const auto& user : users) {
            if (user->authenticate(username, password)) {
                return user;
            }
        }
        return nullptr;
    }

    void displayInventory() {
        cout << "\nBookstore Inventory:\n";
        cout << "====================\n";
        for (const auto& book : inventory) {
            cout << book->getDescription() << " (Stock: " << book->getStock() << ")\n";
        }
        cout << "====================\n";
    }

    void displayBooksByCategory(const string& category) {
        cout << "\n" << category << " Books:\n";
        cout << "====================\n";
        for (const auto& book : inventory) {
            if (book->getCategory() == category) {
                cout << book->getDescription() << " (Stock: " << book->getStock() << ")\n";
            }
        }
        cout << "====================\n";
    }
};

Bookstore* Bookstore::instance = nullptr;

// AI Teacher Class (Singleton)
class AITeacher {
private:
    static AITeacher* instance;
    map<string, vector<string>> knowledgeBase;

    AITeacher() {
        // Initialize knowledge base
        knowledgeBase["academic"] = {
            "Academic books focus on educational content for various subjects.",
            "These books are great for students and researchers.",
            "They typically contain factual information and research findings."
        };
        
        knowledgeBase["fiction"] = {
            "Fiction books contain imaginative stories and narratives.",
            "They are great for entertainment and developing creativity.",
            "Fiction includes genres like mystery, sci-fi, and romance."
        };
        
        knowledgeBase["religious"] = {
            "Religious books contain spiritual teachings and beliefs.",
            "They provide guidance on faith and moral values.",
            "These books are important for religious studies and personal growth."
        };
    }

public:
    static AITeacher* getInstance() {
        if (!instance) {
            instance = new AITeacher();
        }
        return instance;
    }

    string answerQuestion(const string& category, const string& question) {
        if (knowledgeBase.find(category) != knowledgeBase.end()) {
            // Simple response - in a real app, this would use NLP
            return "Regarding " + category + " books: " + 
                   knowledgeBase[category][rand() % knowledgeBase[category].size()];
        }
        return "I'm not sure about that topic. Can you ask about Academic, Fiction, or Religious books?";
    }

    string recommendBook(const string& category) {
        auto bookstore = Bookstore::getInstance();
        auto books = bookstore->getBooksByCategory(category);
        
        if (!books.empty()) {
            auto book = books[rand() % books.size()];
            return "I recommend: " + book->getDescription();
        }
        
        return "I don't have any recommendations for " + category + " books right now.";
    }
};

AITeacher* AITeacher::instance = nullptr;

// Main Application
int main() {
    auto bookstore = Bookstore::getInstance();
    auto aiTeacher = AITeacher::getInstance();

    cout << "Welcome to ChatBook!\n";

    // Sample user interaction
    string username, password;
    cout << "Login\n";
    cout << "Username: ";
    cin >> username;
    cout << "Password: ";
    cin >> password;

    auto user = bookstore->authenticateUser(username, password);
    if (!user) {
        cout << "Invalid credentials!\n";
        return 1;
    }

    cout << "\nWelcome, " << user->getUsername() << " (" << user->getRole() << ")\n";

    if (user->getRole() == "Customer") {
        auto customer = dynamic_pointer_cast<Customer>(user);
        ShoppingCart& cart = customer->getCart();

        // Display books by category
        bookstore->displayBooksByCategory("Academic");
        bookstore->displayBooksByCategory("Fiction");
        bookstore->displayBooksByCategory("Religious");

        // Sample interaction
        cout << "\nAI Teacher is ready to help. Ask a question or request a recommendation.\n";
        string input;
        cin.ignore(); // Clear buffer
        getline(cin, input);

        if (input.find("recommend") != string::npos) {
            if (input.find("academic") != string::npos) {
                cout << aiTeacher->recommendBook("Academic") << endl;
            } else if (input.find("fiction") != string::npos) {
                cout << aiTeacher->recommendBook("Fiction") << endl;
            } else if (input.find("religious") != string::npos) {
                cout << aiTeacher->recommendBook("Religious") << endl;
            } else {
                cout << "Please specify a category (Academic, Fiction, or Religious)\n";
            }
        } else {
            cout << aiTeacher->answerQuestion("general", input) << endl;
        }

        // Add some books to cart
        auto books = bookstore->getBooksByCategory("Academic");
        cart.addItem(books[0]); // First academic book
        cart.addItem(books[1], 2); // Second academic book, 2 copies

        books = bookstore->getBooksByCategory("Fiction");
        cart.addItem(books[0]); // First fiction book

        // View cart
        customer->viewCart();

        // Checkout
        cout << "Proceeding to checkout...\n";
        cart.checkout();
        cout << "Thank you for your purchase!\n";

    } else if (user->getRole() == "Admin") {
        auto admin = dynamic_pointer_cast<Admin>(user);
        cout << "\nAdmin Dashboard\n";
        bookstore->displayInventory();

        // Admin can add new books, update prices, etc.
        cout << "\nAdding a new book...\n";
        auto newBook = make_shared<AcademicBook>("Advanced Physics", "Dr. Newton", "9783123456789", 2000.0f, 3);
        admin->addBook(bookstore->getInventory(), newBook);

        cout << "\nUpdating book price...\n";
        admin->updateBookPrice(bookstore->getInventory(), "9780123456789", 1700.0f);

        cout << "\nRestocking a book...\n";
        admin->restockBook(bookstore->getInventory(), "9780123456790", 5);

        bookstore->displayInventory();
    }

    return 0;
}
