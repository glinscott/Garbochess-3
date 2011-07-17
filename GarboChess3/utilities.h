#include <vector>

std::vector<std::string> tokenize(const std::string &in, const std::string &tok);
Move MakeMoveFromUciString(Position &position, const std::string &moveString);
std::string GetSquareSAN(const Square square);
std::string GetMoveUci(const Move move);
std::string GetMoveSAN(Position &position, const Move move);