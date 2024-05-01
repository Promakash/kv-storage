namespace Utils {
    std::string GenerateRandomString(int length) {
        const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        const int charactersLength = characters.length();
        std::string randomString;

        for (int i = 0; i < length; ++i) {
            randomString += characters[std::rand() % charactersLength];
        }

        return randomString;
    }
}