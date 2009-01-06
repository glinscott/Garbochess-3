#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "search.h"
#include "evaluation.h"
#include "hashtable.h"

#include <cstdlib>

template<class T>
void Swap(T& a, T &b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

const int MoveSentinelScore = MinEval - 1;

// TODO: make this use contempt?
const int DrawScore = 0;

// TODO: Use PSQ tables in SEE?
static const int seePieceValues[8] = { 0, 1, 3, 3, 5, 10, 10000, 0 };

// This version of SEE does not calculate the exact material imbalance, it just returns true = winning or equal, false = losing
bool FastSee(const Position &position, const Move move, const Color us)
{
	const Square from = GetFrom(move);
	const Square to = GetTo(move);

	const Color them = FlipColor(us);

	ASSERT(position.Board[from] != PIECE_NONE);
	ASSERT(GetPieceColor(position.Board[from]) == us);
	ASSERT(position.Board[to] == PIECE_NONE || GetPieceColor(position.Board[from]) != GetPieceColor(position.Board[to]));

	const int fromValue = seePieceValues[GetPieceType(position.Board[from])];
	const int toValue = seePieceValues[GetPieceType(position.Board[to])];

	if (fromValue <= toValue)
	{
		return true;
	}

	if (GetMoveType(move) == MoveTypeEnPassent)
	{
		// e.p. captures are always pxp which is winning or equal.
		return true;
	}

	// We should only be making initially losing captures here
	ASSERT(fromValue > seePieceValues[PAWN] || GetMoveType(move) == MoveTypePromotion || toValue == 0);

	const Bitboard enemyPieces = position.Colors[them];

	// Pawn attacks
	// If any opponent pawns can capture back, this capture is not worthwhile.
	// This is because any pawn captures will return true above, so we must be using a knight or above.
	if (GetPawnAttacks(to, us) & position.Pieces[PAWN] & enemyPieces)
	{
		return false;
	}

	// Knight attacks
	Bitboard attackingPieces = GetKnightAttacks(to) & position.Pieces[KNIGHT];
	const int captureDeficit = fromValue - toValue;
	// If any opponent knights can capture back, and the deficit we have to make up is greater than the knights value,
	// it's not worth it.  We can capture on this square again, and the opponent doesn't have to capture back.
	if (captureDeficit > seePieceValues[KNIGHT] && (attackingPieces & enemyPieces))
	{
		return false;
	}

	// Bishop attacks
	Bitboard allPieces = position.Colors[us] | enemyPieces;
	XorClearBit(allPieces, from);

	attackingPieces |= GetBishopAttacks(to, allPieces) & (position.Pieces[BISHOP] | position.Pieces[QUEEN]);
	if (captureDeficit > seePieceValues[BISHOP] && (attackingPieces & position.Pieces[BISHOP] & enemyPieces))
	{
		return false;
	}

	// Pawn defenses
	// At this point, we are sure we are making a "losing" capture.  The opponent can not capture back with a
	// pawn.  They cannot capture back with a bishop/knight and stand pat either.  So, if we can capture with
	// a pawn, it's got to be a winning or equal capture.
	if (GetPawnAttacks(to, them) & position.Pieces[PAWN] & position.Colors[us])
	{
		return true;
	}

	// Rook attacks
	attackingPieces |= GetRookAttacks(to, allPieces) & (position.Pieces[ROOK] | position.Pieces[QUEEN]);
	if (captureDeficit > seePieceValues[ROOK] && (attackingPieces & position.Pieces[ROOK] & enemyPieces))
	{
		return false;
	}

	// King attacks
	attackingPieces |= GetKingAttacks(to) & position.Pieces[KING];

	// Make sure our original attacking piece is not included in the set of attacking pieces
	attackingPieces &= allPieces;

	if (!attackingPieces)
	{
		// No pieces attacking us, we have won
		return true;
	}

	// At this point, we have built a bitboard of all the pieces attacking the "to" square.  This includes
	// potential x-ray attacks from removing the "from" square piece, which we used to do the capture.

	// We are currently winning the amount of material of the captured piece, time to see if the opponent
	// can get it back somehow.  We assume the opponent can capture our current piece in this score, which
	// simplifies the later code considerably.
	int seeValue = toValue - fromValue;

	for (;;)
	{
		int capturingPieceValue = -1;

		// Find the least valuable piece of the opponent that can attack the square
		for (PieceType capturingPiece = KNIGHT; capturingPiece <= KING; capturingPiece++)
		{
			const Bitboard attacks = position.Pieces[capturingPiece] & attackingPieces & position.Colors[them];
			if (attacks)
			{
				// They can capture with a piece
				capturingPieceValue = seePieceValues[capturingPiece];
				
				const Square attackingSquare = GetFirstBitIndex(attacks);
				XorClearBit(attackingPieces, attackingSquare);
				XorClearBit(allPieces, attackingSquare);
				break;
			}
		}

		if (capturingPieceValue == -1)
		{
			// The opponent can't capture back - we win
			return true;
		}

		// Now, if seeValue < 0, the opponent is winning.  If even after we take their piece,
		// we can't bring it back to 0, then we have lost this battle.
		seeValue += capturingPieceValue;
		if (seeValue < 0)
		{
			return false;
		}

		// Add any x-ray attackers
		attackingPieces |= 
			((GetBishopAttacks(to, allPieces) & (position.Pieces[BISHOP] | position.Pieces[QUEEN])) |
			(GetRookAttacks(to, allPieces) & (position.Pieces[ROOK] | position.Pieces[QUEEN]))) & allPieces;

		// Our turn to capture
		capturingPieceValue = -1;

		// Find the least valuable piece of the opponent that can attack the square
		for (PieceType capturingPiece = KNIGHT; capturingPiece <= KING; capturingPiece++)
		{
			const Bitboard attacks = position.Pieces[capturingPiece] & attackingPieces & position.Colors[us];
			if (attacks)
			{
				// We can capture with a piece
				capturingPieceValue = seePieceValues[capturingPiece];
				
				const Square attackingSquare = GetFirstBitIndex(attacks);
				XorClearBit(attackingPieces, attackingSquare);
				XorClearBit(allPieces, attackingSquare);
				break;
			}
		}

		if (capturingPieceValue == -1)
		{
			// We can't capture back, we lose :(
			return false;
		}

		// Assume our opponent can capture us back, and if we are still winning, we can stand-pat
		// here, and assume we've won.
		seeValue -= capturingPieceValue;
		if (seeValue >= 0)
		{
			return true;
		}

		// Add any x-ray attackers
		attackingPieces |= 
			((GetBishopAttacks(to, allPieces) & (position.Pieces[BISHOP] | position.Pieces[QUEEN])) |
			(GetRookAttacks(to, allPieces) & (position.Pieces[ROOK] | position.Pieces[QUEEN]))) & allPieces;

	}
}

void StableSortMoves(Move *moves, int *moveScores, int moveCount)
{
	// Stable sort the moves
	for (int i = 1; i < moveCount; i++)
	{
		int value = moveScores[i];
		Move move = moves[i];

		int j = i - 1;
		for (; j >= 0 && moveScores[j] < value; j--)
		{
			moveScores[j + 1] = moveScores[j];
			moves[j + 1] = moves[j];
		}

		moveScores[j + 1] = value;
		moves[j + 1] = move;
	}
}

int History[16][64];

enum MoveGenerationState
{
	MoveGenerationState_Hash,
	MoveGenerationState_GenerateWinningEqualCaptures,
	MoveGenerationState_WinningEqualCaptures,
	MoveGenerationState_Killer1,
	MoveGenerationState_Killer2,
	MoveGenerationState_GenerateQuietMoves,
	MoveGenerationState_QuietMoves,
	MoveGenerationState_GenerateLosingCaptures,
	MoveGenerationState_LosingCaptures,
	MoveGenerationState_CheckEscapes,
};

template<int maxMoves>
class MoveSorter
{
public:
	MoveSorter(const Position &position) : position(position)
	{
	}

	inline int GetMoveCount() const
	{
		return moveCount;
	}

	inline MoveGenerationState GetMoveGenerationState() const
	{
		return state;
	}

	inline void GenerateCaptures()
	{
		at = 0;
		moveCount = GenerateCaptureMoves(position, moves, moveScores);
		moves[moveCount] = 0; // Sentinel move
	}

	inline void GenerateCheckEscape()
	{
		at = 0;
		moveCount = GenerateCheckEscapeMoves(position, moves);
		moves[moveCount] = 0;

		state = MoveGenerationState_CheckEscapes;

		for (int i = 0; i < moveCount; i++)
		{
			const PieceType toPiece = GetPieceType(position.Board[GetTo(moves[i])]);

			moveScores[i] = toPiece != PIECE_NONE ? 
				// Sort captures first
				ScoreCaptureMove(moves[i], GetPieceType(position.Board[GetFrom(moves[i])]), toPiece) :
				// Score non-captures as equal
				0;
		}
	}

	inline void GenerateChecks()
	{
		at = 0;
		moveCount = GenerateCheckingMoves(position, moves);
		moves[moveCount] = 0;

		// Disabled since we don't order our checking moves.
		/*
		for (int i = 0; i < moveCount; i++)
		{
			moveScores[i] = 0;
		}
		*/
	}

	inline void InitializeNormalMoves(const Move hashMove, const Move killer1, const Move killer2)
	{
		at = 0;
		state = hashMove == 0 ? MoveGenerationState_GenerateWinningEqualCaptures : MoveGenerationState_Hash;

		this->hashMove = hashMove;
		this->killer1 = killer1;
		this->killer2 = killer2;
	}

	inline Move PickBestMove()
	{
		// We use a trick here instead of checking for at == moveCount.
		// The move initialization code stores a sentinel move at the end of the movelist.
		ASSERT(at <= moveCount);

		s16 bestScore = moveScores[at], bestMove = moves[at];
		for (int i = ++at; i < moveCount; i++)
		{
			ASSERT(moveScores[i] >= MinEval && moveScores[i] <= MaxEval);

			const s16 score = moveScores[i];
			if (score > bestScore)
			{
				const Move move = moves[i];
				moves[i] = bestMove;
				moveScores[i] = bestScore;
				bestMove = move;
				bestScore = score;
			}
		}

		ASSERT(at >= moveCount || (moveScores[at - 1] >= moveScores[at]));

		return bestMove;
	}

	inline Move NextQMove()
	{
		return PickBestMove();
	}

	inline Move NextNormalMove()
	{
		ASSERT(state >= MoveGenerationState_Hash && state <= MoveGenerationState_CheckEscapes);

		switch (state)
		{
		case MoveGenerationState_Hash:
			if (IsMovePseudoLegal(position, hashMove))
			{
				state = MoveGenerationState_GenerateWinningEqualCaptures;
				return hashMove;
			}

			// Intentional fall-through

		case MoveGenerationState_GenerateWinningEqualCaptures:
			GenerateCaptures();
			losingCapturesCount = 0;

			state = MoveGenerationState_WinningEqualCaptures;

			// Intentional fall-through

		case MoveGenerationState_WinningEqualCaptures:
			while (at < moveCount)
			{
				const Move bestMove = PickBestMove();
				ASSERT(bestMove != 0);

				if (bestMove == hashMove)
				{
					continue;
				}

				// Losing captures go off to the back of the list, winning/equal get returned.  They will
				// have been scored correctly already.
				if (FastSee(position, bestMove, position.ToMove))
				{
					return bestMove;
				}
				else
				{
					losingCaptures[losingCapturesCount++] = bestMove;
				}
			}

			// Intentional fall-through

		case MoveGenerationState_Killer1:
			if (killer1 != hashMove && IsMovePseudoLegal(position, killer1))
			{
				state = MoveGenerationState_Killer2;
				return killer1;
			}

			// Intentional fall-through

		case MoveGenerationState_Killer2:
			if (killer2 != hashMove && IsMovePseudoLegal(position, killer2))
			{
				state = MoveGenerationState_GenerateQuietMoves;
				return killer2;
			}
			
			// Intentional fall-through

		case MoveGenerationState_GenerateQuietMoves:
			moveCount = GenerateQuietMoves(position, moves);
			at = 0;

			for (int i = 0; i < moveCount; i++)
			{
				int historyScore = History[position.Board[GetFrom(moves[i])]][GetTo(moves[i])];
				if (historyScore > 32767) historyScore = 32767;
				if (historyScore < -32767) historyScore = -32767;
				moveScores[i] = historyScore;
			}

			state = MoveGenerationState_QuietMoves;
			// Intentional fall-through

		case MoveGenerationState_QuietMoves:
			while (at < moveCount)
			{
				const Move bestMove = PickBestMove();
				if (bestMove == hashMove || bestMove == killer1 || bestMove == killer2)
				{
					continue;
				}
				return bestMove;
			}

			// Intentional fall-through

		case MoveGenerationState_GenerateLosingCaptures:
			at = 0;
			state = MoveGenerationState_LosingCaptures;

			// Intentional fall-through

		case MoveGenerationState_LosingCaptures:
			while (at < losingCapturesCount)
			{
				const Move bestMove = losingCaptures[at++];
				if (bestMove == hashMove)
				{
					continue;
				}
				return bestMove;
			}
			break;

		case MoveGenerationState_CheckEscapes:
			return PickBestMove();
		}

		return 0;
	}

private:

	Move moves[maxMoves];
	s16 moveScores[maxMoves];
	int moveCount;

	Move losingCaptures[32];
	int losingCapturesCount;

	int at;
	const Position &position;
	MoveGenerationState state;
	Move hashMove, killer1, killer2;
};

// The time the search was begun at
u64 SearchStartTime;
u64 SearchTimeLimit;

bool KillSearch;
jmp_buf KillSearchJump;

const int SearchInfoPageSize = 4096;
u64 searchInfoThreads;

SearchInfo &GetSearchInfo(int thread)
{
	return (SearchInfo&)*((SearchInfo*)(searchInfoThreads + (SearchInfoPageSize * thread)));
}

const int qPruningWeight[8] = { 900, 100, 325, 325, 500, 900, 0, 0 };

// depth == 0 is normally what is called for q-search.
// Checks are searched when depth >= -(OnePly / 2).  Depth is decreased by 1 for checks
int QSearch(Position &position, SearchInfo &searchInfo, int alpha, const int beta, const int depth)
{
	ASSERT(!position.IsInCheck());

	searchInfo.QNodeCount++;

	if (position.IsDraw())
	{
		return DrawScore;
	}

	// What do we want from our evaluation? - this needs to be decided (mobility/threat information?)
	EvalInfo evalInfo;
	int eval = Evaluate(position, evalInfo);

	if (eval > alpha)
	{
		alpha = eval;
		if (alpha >= beta)
		{
			return eval;
		}
	}

	const int optimisticValue = eval + 100;

	MoveSorter<64> moves(position);
	moves.GenerateCaptures();

	Move move;
	while ((move = moves.NextQMove()) != 0)
	{
		if (depth <= 0 && !FastSee(position, move, position.ToMove))
		{
			// TODO: we are excluding losing checks here as well - is this safe?
			continue;
		}

		const int pruneValue = optimisticValue + qPruningWeight[GetPieceType(position.Board[GetTo(move)])];

		MoveUndo moveUndo;
		position.MakeMove(move, moveUndo);

		if (!position.CanCaptureKing())
		{
			int value;

			// Search this move
			if (position.IsInCheck())
			{
				value = -QSearchCheck(position, searchInfo, -beta, -alpha, depth - 1);
			}
			else
			{
				if (pruneValue <= alpha &&
					alpha == beta - 1)
				{
					value = pruneValue;
				}
				else
				{
					value = -QSearch(position, searchInfo, -beta, -alpha, depth - OnePly);
				}
			}

			position.UnmakeMove(move, moveUndo);

			if (value > eval)
			{
				eval = value;
				if (value > alpha)
				{
					alpha = value;
					if (value >= beta)
					{
						return value;
					}
				}
			}
		}
		else
		{
			position.UnmakeMove(move, moveUndo);
		}
	}

	if (depth < -6)
	{
		// Don't bother searching checking moves
		return eval;
	}

	moves.GenerateChecks();

	while ((move = moves.NextQMove()) != 0)
	{
		if (depth <= 0 && !FastSee(position, move, position.ToMove))
		{
			continue;
		}

		MoveUndo moveUndo;
		position.MakeMove(move, moveUndo);

		if (!position.CanCaptureKing())
		{
			ASSERT(position.IsInCheck());

			int value = -QSearchCheck(position, searchInfo, -beta, -alpha, depth - 1);

			position.UnmakeMove(move, moveUndo);
			
			if (value > eval)
			{
				eval = value;
				if (value > alpha)
				{
					alpha = value;
					if (value >= beta)
					{
						return beta;
					}
				}
			}
		}
		else
		{
			position.UnmakeMove(move, moveUndo);
		}
	}
	
	return eval;
}

int QSearchCheck(Position &position, SearchInfo &searchInfo, int alpha, const int beta, const int depth)
{
	ASSERT(position.IsInCheck());

	searchInfo.QNodeCount++;

	if (position.IsDraw())
	{
		return DrawScore;
	}

	MoveSorter<64> moves(position);
	moves.GenerateCheckEscape();

	if (moves.GetMoveCount() == 0)
	{
		// Checkmate
		return MinEval;
	}

	int bestScore = MoveSentinelScore;

	// Single-reply to check extension
	const int depthReduction = moves.GetMoveCount() == 1 ? 1 : OnePly;

	Move move;
	while ((move = moves.NextQMove()) != 0)
	{
		MoveUndo moveUndo;
		position.MakeMove(move, moveUndo);

		ASSERT(!position.CanCaptureKing());

		int value;
		if (position.IsInCheck())
		{
			value = -QSearchCheck(position, searchInfo, -beta, -alpha, depth - 1);
		}
		else
		{
			value = -QSearch(position, searchInfo, -beta, -alpha, depth - depthReduction);
		}

		position.UnmakeMove(move, moveUndo);

		if (value > bestScore)
		{
			bestScore = value;
			if (value > alpha)
			{
				alpha = value;
				if (value >= beta)
				{
					return value;
				}
			}
		}
	}

	ASSERT(bestScore != MoveSentinelScore);

	return bestScore;
}

bool CheckElapsedTime()
{
	u64 elapsedTime = GetCurrentMilliseconds() - SearchStartTime;
	if (elapsedTime >= SearchTimeLimit)
	{
		return true;
	}

	return false;
}

void CheckKillSearch()
{
	void ReadCommand();

	while (!KillSearch && CheckForPendingInput())
	{
		ReadCommand();
	}

	if (CheckElapsedTime())
	{
		KillSearch = true;
	}

	if (KillSearch)
	{
		longjmp(KillSearchJump, 1);
	}
}

int Search(Position &position, SearchInfo &searchInfo, const int beta, const int ply, const int depthFromRoot, const int flags, const bool inCheck)
{
	ASSERT(ply > 0);
	ASSERT(inCheck ? position.IsInCheck() : !position.IsInCheck());

	searchInfo.NodeCount++;

	if (position.IsDraw())
	{
		return DrawScore;
	}

	HashEntry *hashEntry;
	Move hashMove;
	if (ProbeHash(position.Hash, hashEntry))
	{
		if (hashEntry->Depth >= (ply / OnePly))
		{
			const int hashFlags = hashEntry->GetHashFlags();
			if (hashFlags == HashFlagsExact)
			{
				return hashEntry->Score;
			}
			else if (hashEntry->Score >= beta && hashFlags == HashFlagsBeta)
			{
				return hashEntry->Score;
			}
			else if (hashEntry->Score < beta && hashFlags == HashFlagsAlpha)
			{
				return hashEntry->Score;
			}
		}

		hashMove = hashEntry->Move;
	}
	else
	{
		hashMove = 0;
	}

	int evaluation;

	if (!inCheck)
	{
		EvalInfo evalInfo;
		evaluation = Evaluate(position, evalInfo);

		int razorEval = evaluation + 125;
		if (razorEval < beta)
		{
			// Try some razoring
			if (ply <= OnePly)
			{
				int value = QSearch(position, searchInfo, beta - 1, beta, ply - OnePly);
				return max(value, razorEval);
			}
			razorEval += 175;
			if (razorEval < beta && ply <= OnePly * 3)
			{
				int value = QSearch(position, searchInfo, beta - 1, beta, ply - OnePly);
				if (value < beta)
				{
					return max(value, razorEval);
				}
			}
		}

		if (ply >= 2 * OnePly &&
			evaluation >= beta &&
			!(flags & 1) &&
			// Make sure we don't null move if we don't have any heavy pieces left
			evalInfo.GamePhase[position.ToMove] > 0)
		{
			// Attempt to null-move
			MoveUndo moveUndo;
			position.MakeNullMove(moveUndo);

			//const int R = (ply >= 7 * OnePly) ? (5 * OnePly) : (4 * OnePly);
			const int R = 4 * OnePly;

			const int newPly = ply - R;
			int score;
			if (newPly <= 0)
			{
				score = -QSearch(position, searchInfo, -beta, 1 - beta, 0);
			}
			else
			{
				score = -Search(position, searchInfo, 1 - beta, newPly, depthFromRoot + 1, 1, false);
			}

			position.UnmakeNullMove(moveUndo);

			// Null move verification
			if (ply > 5 * OnePly &&
				score >= beta)
			{
				score = Search(position, searchInfo, beta, ply - (5 * OnePly), depthFromRoot + 1, 1, false);
			}

			if (score >= beta)
			{
				StoreHash(position.Hash, score, 0, ply, HashFlagsBeta);
				return score;
			}
		}
	}

	MoveSorter<256> moves(position);
	bool singular = false;
	if (!inCheck)
	{
		moves.InitializeNormalMoves(hashMove, searchInfo.Killers[depthFromRoot][0], searchInfo.Killers[depthFromRoot][1]);
	}
	else
	{
		moves.GenerateCheckEscape();
		if (moves.GetMoveCount() == 0)
		{
			return MinEval;
		}
		else if (moves.GetMoveCount() == 1)
		{
			singular = true;
		}
	}

	int moveCount = 0;
	int bestScore = MoveSentinelScore;
	Move move;
	while ((move = moves.NextNormalMove()) != 0)
	{
		MoveUndo moveUndo;
		position.MakeMove(move, moveUndo);

		if (!position.CanCaptureKing())
		{
			int value;

			// Search move
			const bool isChecking = position.IsInCheck();
			int newPly;
			if (isChecking || singular)
			{
				newPly = ply;
			}
			else
			{
				// Try futility pruning
				if (!inCheck &&
					moves.GetMoveGenerationState() == MoveGenerationState_QuietMoves &&
					GetPieceType(position.Board[GetTo(move)]) != PAWN &&
					ply <= 2 * OnePly)
				{
					if (ply <= OnePly * 2)
					{
						value = evaluation + 100;
					}
					else 
					{
						value = evaluation + 250;
					}

					if (value < beta)
					{
						position.UnmakeMove(move, moveUndo);
			
						if (value > bestScore)
						{
							bestScore = value;
							hashMove = move;
						}
						continue;
					}
				}

				// Apply late move reductions if the conditions are met.
				if (!inCheck &&
					moves.GetMoveGenerationState() == MoveGenerationState_QuietMoves &&
					moveCount >= 4 &&
					ply >= 3 * OnePly)
				{
					if (moveCount >= 25 &&
						GetPieceType(position.Board[GetTo(move)]) != PAWN)
					{
						newPly = ply - ((5 * OnePly) / 2);
					}
					else
					{
						newPly = ply - (2 * OnePly);
					}
				}
				else
				{
					newPly = ply - OnePly;
				}
			}

			if (newPly <= 0)
			{
				ASSERT(!isChecking);
				value = -QSearch(position, searchInfo, -beta, 1 - beta, 0);
			}
			else
			{
				value = -Search(position, searchInfo, 1 - beta, newPly, depthFromRoot + 1, 0, isChecking);
			}

			if (newPly <= ply - (2 * OnePly) && value >= beta)
			{
				// Re-search if the reduced move actually has the potential to be a good move.
				ASSERT(!isChecking);
				ASSERT(!inCheck);

				newPly = ply - OnePly;
				ASSERT(newPly > 0);

				value = -Search(position, searchInfo, 1 - beta, newPly, depthFromRoot + 1, 0, isChecking);
			}

			position.UnmakeMove(move, moveUndo);

			moveCount++;

			if (value > bestScore)
			{
				bestScore = value;
				hashMove = move;

				if (value >= beta)
				{
					StoreHash(position.Hash, value, move, ply, HashFlagsBeta);

					// Update killers and history (only for non-captures)
					const Square to = GetTo(move);
					if (position.Board[to] == PIECE_NONE)
					{
						if (move != searchInfo.Killers[depthFromRoot][0] &&
							GetMoveType(move) != MoveTypePromotion)
						{
							searchInfo.Killers[depthFromRoot][1] = searchInfo.Killers[depthFromRoot][0];
							searchInfo.Killers[depthFromRoot][0] = move;
						}

						// Update history board, which is [pieceType][to], to allow for better move ordering
						const int normalizedPly = ply / OnePly;
						const Square from = GetFrom(move);
						History[position.Board[from]][to] += normalizedPly * normalizedPly;
						if (History[position.Board[from]][to] > 200000)
						{
							for (int i = 0; i < 16; i++)
							{
								for (int j = 0; j < 64; j++)
								{
									History[i][j] /= 8;
								}
							}
						}
					}

					return value;
				}
			}
		}
		else
		{
			position.UnmakeMove(move, moveUndo);
		}
	}

	if (bestScore == MoveSentinelScore)
	{
		ASSERT(!inCheck);
		// Stalemate
		return DrawScore;
	}

	// TODO: some sort of history update here?

	StoreHash(position.Hash, bestScore, hashMove, ply, HashFlagsAlpha);

	if (searchInfo.NodeCount + searchInfo.QNodeCount > searchInfo.Timeout)
	{
		searchInfo.Timeout = searchInfo.NodeCount + searchInfo.QNodeCount + 30000;
		CheckKillSearch();
	}

	return bestScore;
}

int SearchPV(Position &position, SearchInfo &searchInfo, int alpha, const int beta, const int ply, const int depthFromRoot, const bool inCheck)
{
	ASSERT(inCheck ? position.IsInCheck() : !position.IsInCheck());

	// TODO: should probably count PV nodes differently
	searchInfo.NodeCount++;

	if (ply <= 0)
	{
		ASSERT(!inCheck);
		return QSearch(position, searchInfo, alpha, beta, 0);
	}

	if (position.IsDraw())
	{
		return DrawScore;
	}

	HashEntry *hashEntry;
	Move hashMove;
	if (ProbeHash(position.Hash, hashEntry))
	{
		hashMove = hashEntry->Move;
	}
	else
	{
		hashMove = 0;
	}

	if (ply >= OnePly * 2 &&
		hashMove == 0)
	{
		// Try IID
		SearchPV(position, searchInfo, alpha, beta, max(OnePly, min(ply - (2 * OnePly), ply / 2)), depthFromRoot + 1, inCheck);

		// TODO: re-search if < alpha here?  that means we won't have a best move...

		if (ProbeHash(position.Hash, hashEntry))
		{
			hashMove = hashEntry->Move;
		}
	}

	// TODO: use root move sorting in PV positions?
	MoveSorter<256> moves(position);
	bool singular = false;

	if (!inCheck)
	{
		moves.InitializeNormalMoves(hashMove, searchInfo.Killers[depthFromRoot][0], searchInfo.Killers[depthFromRoot][1]);
	}
	else
	{
		moves.GenerateCheckEscape();
		if (moves.GetMoveCount() == 0)
		{
			//TODO: Make checkmates return correct "depth"
			return MinEval;
		}
		else if (moves.GetMoveCount() == 1)
		{
			singular = true;
		}
	}

	const int originalAlpha = alpha;
	int bestScore = MoveSentinelScore;

	Move move;
	while ((move = moves.NextNormalMove()) != 0)
	{
		MoveUndo moveUndo;
		position.MakeMove(move, moveUndo);

		if (!position.CanCaptureKing())
		{
			int value;

			// Search move
			const bool isChecking = position.IsInCheck();
			const int newPly = (isChecking || singular) ? ply : ply - OnePly;

			if (bestScore == MoveSentinelScore)
			{
				value = -SearchPV(position, searchInfo, -beta, -alpha, newPly, depthFromRoot + 1, isChecking);
			}
			else
			{
				if (newPly <= 0)
				{
					value = -QSearch(position, searchInfo, -alpha - 1, -alpha, 0);
				}
				else
				{
					value = -Search(position, searchInfo, -alpha, newPly, depthFromRoot + 1, 0, isChecking);
				}

				if (value > alpha)
				{
					value = -SearchPV(position, searchInfo, -beta, -alpha, newPly, depthFromRoot + 1, isChecking);
				}
			}

			position.UnmakeMove(move, moveUndo);

			if (value > bestScore)
			{
				bestScore = value;
				hashMove = move;

				if (value > alpha)
				{
					alpha = value;

					if (value >= beta)
					{
						// TODO: update history
						StoreHash(position.Hash, value, move, ply, HashFlagsBeta);

						// Update killers (only for non-captures/promotions)
						if (position.Board[GetTo(move)] == PIECE_NONE &&
							GetMoveType(move) != MoveTypePromotion &&
							move != searchInfo.Killers[depthFromRoot][0])
						{
							searchInfo.Killers[depthFromRoot][1] = searchInfo.Killers[depthFromRoot][0];
							searchInfo.Killers[depthFromRoot][0] = move;
						}

						return value;
					}
				}
			}
		}
		else
		{
			position.UnmakeMove(move, moveUndo);
		}
	}

	if (bestScore == MoveSentinelScore)
	{
		ASSERT(!inCheck);
		// Stalemate
		return DrawScore;
	}

	StoreHash(position.Hash, bestScore, hashMove, ply, bestScore > originalAlpha ? HashFlagsExact : HashFlagsAlpha);

	return bestScore;
}

int SearchRoot(Position &position, SearchInfo &searchInfo, Move *moves, int *moveScores, int moveCount, int alpha, const int beta, const int depth)
{
	ASSERT(depth % OnePly == 0);

	// This is a bit of a hack, but it's cleaner than checking everywhere that we have terminated the search cleanly.
	if (setjmp(KillSearchJump) != 0)
	{
		return MinEval;
	}

	memset(History, 0, sizeof(History));

	int originalAlpha = alpha;
	int bestScore = MoveSentinelScore;

	for (int i = 0; i < moveCount; i++)
	{
		MoveUndo moveUndo;
		position.MakeMove(moves[i], moveUndo);

		const bool isChecking = position.IsInCheck();
		const int newDepth = isChecking ? depth : depth - OnePly;
		int value;
		if (bestScore == MoveSentinelScore)
		{
			value = -SearchPV(position, searchInfo, -beta, -alpha, newDepth, 1, isChecking);
		}
		else
		{
			if (newDepth <= 0)
			{
				value = -QSearch(position, searchInfo, -beta, -alpha, newDepth);
			}
			else
			{
				value = -Search(position, searchInfo, -alpha, newDepth, 1, 0, isChecking);
			}

			// Research if value > alpha, as this means this node is a new PV node.
			if (value > alpha)
			{
				value = -SearchPV(position, searchInfo, -beta, -alpha, newDepth, 1, isChecking);
			}
		}

		position.UnmakeMove(moves[i], moveUndo);

		// Update move scores
		if (value <= alpha)
		{
			moveScores[i] = originalAlpha;
		}
		else if (value >= beta)
		{
			moveScores[i] = beta;
		}
		else
		{
			moveScores[i] = value;
		}

		if (value > bestScore)
		{
			bestScore = value;
			if (value > alpha)
			{
				alpha = value;
				if (value >= beta)
				{
					return value;
				}
			}
		}
	}

	return bestScore;
}

void PrintPV(Position &position, const Move move, int depth)
{
	if (depth < 0 || position.IsDraw())
	{
		return;
	}

	printf("%s ", GetMoveSAN(position, move).c_str());

	MoveUndo moveUndo;
	position.MakeMove(move, moveUndo);

	HashEntry *hashEntry;
	if (ProbeHash(position.Hash, hashEntry) &&
		IsMovePseudoLegal(position, hashEntry->Move))
	{
		PrintPV(position, hashEntry->Move, depth - 1);
	}

	position.UnmakeMove(move, moveUndo);
}

Move IterativeDeepening(Position &rootPosition, const int maxDepth, int &score, s64 searchTime, bool printSearchInfo)
{
	KillSearch = false;

	SearchStartTime = GetCurrentMilliseconds();
	if (searchTime < 0)
	{
		SearchTimeLimit = (u64)-1LL;
	}
	else
	{
		SearchTimeLimit = (u64)searchTime;
	}

	Position position;
	rootPosition.Clone(position);

	SearchInfo &searchInfo = GetSearchInfo(0);
	searchInfo.NodeCount = 0;
	searchInfo.QNodeCount = 0;
	searchInfo.Timeout = 0;
	// TODO: try tricks with killers? - like moving them down two ply

	Move moves[256];
	int moveCount = GenerateLegalMoves(position, moves);
 
	// Initial sorting of root moves is done by q-search scores
	int moveScores[256];
	for (int i = 0; i < moveCount; i++)
	{
		MoveUndo moveUndo;
		position.MakeMove(moves[i], moveUndo);
		if (position.IsInCheck())
		{
			moveScores[i] = -QSearchCheck(position, searchInfo, MinEval, MaxEval, -OnePly);
		}
		else
		{
			moveScores[i] = -QSearch(position, searchInfo, MinEval, MaxEval, -OnePly);
		}
		position.UnmakeMove(moves[i], moveUndo);
	}

	StableSortMoves(moves, moveScores, moveCount);

	int alpha = MinEval, beta = MaxEval;
	Move bestMove;
	int bestScore;

	// Iterative deepening loop
	for (int depth = 1; depth <= min(maxDepth, 99); depth++)
	{
		for (int i = 0; i < moveCount; i++)
		{
			// Reset moveScores for things we haven't searched yet
			moveScores[i] = MinEval;
		}

		int value = SearchRoot(position, searchInfo, moves, moveScores, moveCount, alpha, beta, depth * OnePly);
/*
		if (value > alpha && value < beta)
		{
			alpha = value - 50;
			beta = value + 50;
		}
		else
		{
			if (value <= alpha && alpha != MinEval)
			{
				alpha = MinEval;
				depth--;
				continue;
			}

			if (value >= beta && beta != MaxEval)
			{
				beta = MaxEval;
				depth--;
				continue;
			}
		}
*/
		if (KillSearch)
		{
			break;
		}

		StableSortMoves(moves, moveScores, moveCount);

		// Best move is the first move in the list
		bestMove = moves[0];
		bestScore = moveScores[0];

		ASSERT(bestScore == value);

		if (printSearchInfo)
		{
			const u64 nodeCount = searchInfo.NodeCount + searchInfo.QNodeCount;
			const u64 msTaken = GetCurrentMilliseconds() - SearchStartTime;
			const u64 nps = (nodeCount * 1000) / max(1ULL, msTaken);
			printf("info depth %d score cp %d nodes %I64d time %I64d nps %I64d pv ", depth, (int)value, nodeCount, msTaken, nps);
			PrintPV(position, moves[0], depth * 3);
			printf("\n");
		}

		// Check timeout here, as sometimes we print too much output without searching enough nodes to run over.
		if (CheckElapsedTime())
		{
			break;
		}
	}

	score = bestScore;
	return bestMove;
}

void InitializeSearch()
{
	ASSERT(sizeof(SearchInfo) < SearchInfoPageSize);

	searchInfoThreads = u64(malloc(SearchInfoPageSize * (MaxThreads + 1)));
	searchInfoThreads += SearchInfoPageSize - (searchInfoThreads & (SearchInfoPageSize - 1));
}

// TODO: check extensions limited by SEE in non-PV nodes?
// TODO: investigate using root move sorting in PV nodes (need PV hash table)
// TODO: investigate heavy pruning of the search tree (>500cp eval = pruning time)