#pragma once
#include <cctype>
#include <string>
#include <string_view>
namespace conversation_face {
// Conservative visual hints only, not a sentiment model. Never synthesizes speech.
inline std::string_view Hint(std::string_view text, bool user) {
    std::string lower(text.substr(0, 1200));
    for (char& ch : lower)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    auto has = [&](std::string_view s) {
        return lower.find(s) != std::string::npos &&
               lower.find(std::string("não ") + std::string(s)) == std::string::npos &&
               lower.find(std::string("nao ") + std::string(s)) == std::string::npos;
    };
    if (user) {
        if (has("estou triste") || has("estou preocupado") || has("estou preocupada") ||
            has("tenho medo") || has("sinto-me sozinho") || has("sinto-me sozinha"))
            return "worried";
        if (has("estou feliz") || has("estou contente") || has("consegui!") ||
            has("consegui passar"))
            return "happy";
    } else {
        if (has("sinto muito") || has("lamento que") || has("imagino que seja difícil"))
            return "worried";
        if (has("parabéns") || has("que bom!") || has("fico feliz por") || has("boa notícia"))
            return "happy";
        if (has("vamos calcular") || has("passo a passo") || has("vamos resolver"))
            return "focused";
    }
    return {};
}
inline bool Neutral(std::string_view emotion) {
    return emotion == "neutral" || emotion == "normal" || emotion == "robot_2";
}
}  // namespace conversation_face
