#include <iostream>
#include <openssl/sha.h>  // Potrebno za SHA-256
#include <random>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <chrono>

std::atomic<bool> found(false);  // Atomarna spremenljivka za sinhronizacijo
std::atomic<int> total_hashes(0);  // Število hash-ov, ki so bili generirani

// Funkcija za generiranje naključnega niza dolžine 256 iz znakov [0-9, a-z]
std::string generate_random_string(size_t length = 64) {
    const std::string characters = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;  // Uporabimo random_device za naključne vrednosti
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> dist(0, characters.size() - 1);

    std::string random_string;
    for (size_t i = 0; i < length; ++i) {
        random_string += characters[dist(generator)];
    }
    return random_string;
}

// Funkcija za izračun SHA-256 hash-a
std::string sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.size());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Funkcija, ki se bo izvajala v niti
void find_matching_string(const std::string& target_hash, int thread_id) {
    while (!found.load()) {  // Nadaljuj, dokler ne najdemo niza
        std::string random_string = generate_random_string();  // Generiraj naključni niz
        std::string random_string_hash = sha256(random_string);  // Izračunaj SHA-256 hash

        total_hashes++;  // Povečaj število generiranih hash-ov

        if (random_string_hash == target_hash) {
            found.store(true);  // Oznaka, da smo našli niz
            std::cout << "Nit " << thread_id << " je našla ujemajoči se niz: " << random_string << std::endl;
            break;
        }
    }
}

// Funkcija za izračun in prikaz hash rate (hash-ov na sekundo)
void hash_rate_counter() {
    while (!found.load()) {
        int previous_hashes = total_hashes.load();
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Počakaj eno sekundo
        int current_hashes = total_hashes.load();
        int hashes_per_second = current_hashes - previous_hashes;
        std::cout << "Hashrate: " << hashes_per_second << " hash-ov na sekundo" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::string target_hash = "c878c9e291c323c0966e77d15588cfd4d74c52808a2e582e9b41003f08b4d6fd";  // Zamenjaj s hash-om, ki ga iščeš
    int numThreads = std::stoi(argv[1]);  // Tukaj določi število niti (lahko ga spremeniš glede na želje)

    // Ustvari nit za spremljanje hashrate
    std::thread hash_rate_thread(hash_rate_counter);

    // Ustvari več niti glede na numThreads
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(find_matching_string, target_hash, i + 1);
    }

    // Počakaj, da se vse niti končajo
    for (auto& t : threads) {
        t.join();
    }

    // Počakaj, da se hash_rate_thread zaključi
    hash_rate_thread.join();

    return 0;
}
