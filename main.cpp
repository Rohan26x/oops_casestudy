#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <cctype>
#include <algorithm>
#include <random>
#include <limits>

using namespace std;

// ============================================================================
// DOMAIN MODELS
// ============================================================================

struct Player {
    string name;
    int lives;
    int score;
    bool isAlive;

    explicit Player(const string& n, int startingLives = 6)
        : name(n), lives(startingLives), score(0), isAlive(true) {}

    void resetForRound(int startingLives = 6) {
        lives = startingLives;
        isAlive = true;
    }
};

// ============================================================================
// 1. WORD PROVIDER ABSTRACTION (SRP, OCP, DIP)
// ============================================================================

class IWordProvider {
public:
    virtual ~IWordProvider() = default;
    virtual string getRandomWord() = 0;
};

class FileWordProvider : public IWordProvider {
private:
    vector<string> words;
    mt19937 rng;

    string cleanWord(const string& str) const {
        string cleaned;
        for (char c : str) {
            if (isalpha(static_cast<unsigned char>(c))) {
                cleaned += static_cast<char>(toupper(static_cast<unsigned char>(c)));
            }
        }
        return cleaned;
    }

public:
    explicit FileWordProvider(const string& filepath)
        : rng(random_device{}()) {
        ifstream file(filepath);
        if (file.is_open()) {
            string line;
            while (getline(file, line)) {
                string word = cleanWord(line);
                if (word.length() >= 3) {
                    words.push_back(word);
                }
            }
        }

        if (words.empty()) {
            words = {};
        }
    }

    string getRandomWord() override {
        uniform_int_distribution<size_t> dist(0, words.size() - 1);
        return words[dist(rng)];
    }
};

// ============================================================================
// 2. SCORING POLICY ABSTRACTION (SRP, OCP, LSP)
// ============================================================================

class IScoringPolicy {
public:
    virtual ~IScoringPolicy() = default;
    virtual int calculateCorrectGuessPoints(int matches) const = 0;
    virtual int calculateWrongGuessPenalty() const = 0;
    virtual int calculateCompletionBonus() const = 0;
};

class StandardScoringPolicy : public IScoringPolicy {
public:
    int calculateCorrectGuessPoints(int matches) const override {
        return matches * 10;
    }

    int calculateWrongGuessPenalty() const override {
        return 2;
    }

    int calculateCompletionBonus() const override {
        return 50;
    }
};

// ============================================================================
// 3. USER INTERFACE ABSTRACTION (SRP, ISP, DIP)
// ============================================================================

class IUserInterface {
public:
    virtual ~IUserInterface() = default;
    virtual int promptPlayerCount() = 0;
    virtual string promptPlayerName(int playerIndex) = 0;
    virtual char promptGuess(const Player& player) = 0;
    virtual bool promptPlayAgain() = 0;

    virtual void showStatus(const string& progress, const vector<Player>& players) = 0;
    virtual void showCorrectGuess(const Player& player, char guess, int matches, int earned) = 0;
    virtual void showIncorrectGuess(const Player& player, char guess, int penalty) = 0;
    virtual void showEliminated(const Player& player) = 0;
    virtual void showDuplicateGuess(char guess) = 0;
    virtual void showInvalidInput() = 0;
    virtual void showRoundEnd(bool won, const string& secretWord) = 0;
    virtual void showLeaderboard(const vector<Player>& players) = 0;
};

class ConsoleUI : public IUserInterface {
public:
    int promptPlayerCount() override {
        int n;
        cout << "Enter number of players: ";
        while (!(cin >> n) || n <= 0) {
            cout << "Please enter a valid positive integer: ";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        return n;
    }

    string promptPlayerName(int playerIndex) override {
        string name;
        cout << "Enter name for Player " << (playerIndex + 1) << ": ";
        cin >> name;
        return name;
    }

    char promptGuess(const Player& player) override {
        cout << ">>> " << player.name << "'s turn (Lives: " << player.lives << ")\n";
        cout << "Enter a letter guess: ";
        char guess;
        cin >> guess;
        return guess;
    }

    bool promptPlayAgain() override {
        cout << "Play another round with the same players? (y/n): ";
        char choice;
        cin >> choice;
        return (choice == 'y' || choice == 'Y');
    }

    void showStatus(const string& progress, const vector<Player>& players) override {
        cout << "\n============================================\nWord: ";
        for (char c : progress) cout << c << ' ';
        cout << "\n\nPlayer Standings:\n";
        for (const auto& p : players) {
            cout << "  - " << p.name << " | Score: " << p.score << " | Lives: "
                 << (p.isAlive ? to_string(p.lives) : "ELIMINATED") << "\n";
        }
        cout << "============================================\n";
    }

    void showCorrectGuess(const Player& player, char guess, int matches, int earned) override {
        cout << "Good job, " << player.name << "! '" << guess << "' appeared "
             << matches << " time(s). (+" << earned << " pts)\n";
    }

    void showIncorrectGuess(const Player& player, char guess, int penalty) override {
        cout << "Incorrect! '" << guess << "' is not in the word. (-1 Life, -" << penalty << " pts)\n";
    }

    void showEliminated(const Player& player) override {
        cout << "--> " << player.name << " has been eliminated from this round!\n";
    }

    void showDuplicateGuess(char guess) override {
        cout << "Letter '" << guess << "' has already been guessed! Turn skipped without penalty.\n";
    }

    void showInvalidInput() override {
        cout << "Invalid input. Please enter an alphabetic character.\n";
    }

    void showRoundEnd(bool won, const string& secretWord) override {
        cout << "\n============================================\n";
        cout << "ROUND OVER! The word was: " << secretWord << "\n";
        cout << (won ? "The puzzle was solved successfully!\n"
                     : "All players ran out of lives! Game over for this round.\n");
        cout << "============================================\n";
    }

    void showLeaderboard(const vector<Player>& players) override {
        vector<Player> ranked = players;
        sort(ranked.begin(), ranked.end(), [](const Player& a, const Player& b) {
            return a.score > b.score;
        });

        cout << "\n=== FINAL LEADERBOARD ===\n";
        for (size_t i = 0; i < ranked.size(); ++i) {
            cout << (i + 1) << ". " << ranked[i].name << " - " << ranked[i].score << " points\n";
        }
        cout << "=========================\n\n";
    }
};

// ============================================================================
// 4. GAME ENGINE (ORCHESTRATOR DEPENDING SOLELY ON ABSTRACTIONS - DIP)
// ============================================================================

class HangmanGame {
private:
    shared_ptr<IWordProvider> wordProvider;
    shared_ptr<IScoringPolicy> scoringPolicy;
    shared_ptr<IUserInterface> ui;

    vector<Player> players;
    string secretWord;
    string currentProgress;
    vector<bool> guessedLetters;
    int activePlayerCount;

    bool isWordGuessed() const {
        return currentProgress == secretWord;
    }

    void prepareNewRound() {
        secretWord = wordProvider->getRandomWord();
        currentProgress = string(secretWord.length(), '_');
        guessedLetters.assign(26, false);
        activePlayerCount = static_cast<int>(players.size());

        for (auto& p : players) {
            p.resetForRound(6);
        }
    }

public:
    HangmanGame(shared_ptr<IWordProvider> wp,
                shared_ptr<IScoringPolicy> sp,
                shared_ptr<IUserInterface> userInterface)
        : wordProvider(move(wp)),
          scoringPolicy(move(sp)),
          ui(move(userInterface)),
          activePlayerCount(0) {}

    void setupPlayers() {
        int count = ui->promptPlayerCount();
        players.clear();
        players.reserve(count);
        for (int i = 0; i < count; ++i) {
            players.emplace_back(ui->promptPlayerName(i));
        }
    }

    void playRound() {
        prepareNewRound();
        size_t turnIndex = 0;

        while (activePlayerCount > 0 && !isWordGuessed()) {
            Player& currentPlayer = players[turnIndex];

            if (!currentPlayer.isAlive) {
                turnIndex = (turnIndex + 1) % players.size();
                continue;
            }

            ui->showStatus(currentProgress, players);

            char rawGuess = ui->promptGuess(currentPlayer);
            if (!isalpha(static_cast<unsigned char>(rawGuess))) {
                ui->showInvalidInput();
                continue;
            }

            char guess = static_cast<char>(toupper(static_cast<unsigned char>(rawGuess)));
            int letterIndex = guess - 'A';

            if (guessedLetters[letterIndex]) {
                ui->showDuplicateGuess(guess);
                turnIndex = (turnIndex + 1) % players.size();
                continue;
            }

            guessedLetters[letterIndex] = true;

            int matches = 0;
            for (size_t i = 0; i < secretWord.length(); ++i) {
                if (secretWord[i] == guess) {
                    currentProgress[i] = guess;
                    matches++;
                }
            }

            if (matches > 0) {
                int earned = scoringPolicy->calculateCorrectGuessPoints(matches);
                currentPlayer.score += earned;
                ui->showCorrectGuess(currentPlayer, guess, matches, earned);

                if (isWordGuessed()) {
                    currentPlayer.score += scoringPolicy->calculateCompletionBonus();
                    break;
                }
            } else {
                int penalty = scoringPolicy->calculateWrongGuessPenalty();
                currentPlayer.lives--;
                currentPlayer.score = max(0, currentPlayer.score - penalty);
                ui->showIncorrectGuess(currentPlayer, guess, penalty);

                if (currentPlayer.lives <= 0) {
                    currentPlayer.isAlive = false;
                    activePlayerCount--;
                    ui->showEliminated(currentPlayer);
                }
            }

            turnIndex = (turnIndex + 1) % players.size();
        }

        ui->showRoundEnd(isWordGuessed(), secretWord);
        ui->showLeaderboard(players);
    }
};

// ============================================================================
// MAIN COMPOSITION ROOT
// ============================================================================

int main() {
    string filename;
    cout << "Enter word list filename (or press Enter for 'words.txt'): ";
    cin >> ws;
    if (cin.peek() == '\n') {
        filename = "words.txt";
    } else {
        getline(cin, filename);
        if (filename.empty()) filename = "words.txt";
    }

    auto wordProvider = make_shared<FileWordProvider>(filename);
    auto scoringPolicy = make_shared<StandardScoringPolicy>();
    auto ui = make_shared<ConsoleUI>();

    HangmanGame game(wordProvider, scoringPolicy, ui);

    game.setupPlayers();

    bool playAgain = true;
    while (playAgain) {
        game.playRound();
        playAgain = ui->promptPlayAgain();
    }

    cout << "Thanks for playing!\n";
    return 0;
}
