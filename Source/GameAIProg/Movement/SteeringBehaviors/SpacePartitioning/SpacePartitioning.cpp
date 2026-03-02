#include "SpacePartitioning.h"

// --- Cell ---
// ------------
Cell::Cell(float Left, float Bottom, float Width, float Height)
{
	BoundingBox.Min = { Left, Bottom };
	BoundingBox.Max = { BoundingBox.Min.X + Width, BoundingBox.Min.Y + Height };
}

std::vector<FVector2D> Cell::GetRectPoints() const
{
	const float left = BoundingBox.Min.X;
	const float bottom = BoundingBox.Min.Y;
	const float width = BoundingBox.Max.X - BoundingBox.Min.X;
	const float height = BoundingBox.Max.Y - BoundingBox.Min.Y;

	std::vector<FVector2D> rectPoints =
	{
		{ left , bottom  },
		{ left , bottom + height  },
		{ left + width , bottom + height },
		{ left + width , bottom  },
	};

	return rectPoints;
}

// --- Partitioned Space ---
// -------------------------
CellSpace::CellSpace(UWorld* pWorld, float Width, float Height, int Rows, int Cols, int MaxEntities)
	: pWorld{pWorld}
	, SpaceWidth{Width}
	, SpaceHeight{Height}
	, NrOfRows{Rows}
	, NrOfCols{Cols}
	, NrOfNeighbors{0}
{
	Neighbors.SetNum(MaxEntities);
	
	//calculate bounds of a cell
	CellWidth = Width / Cols;
	CellHeight = Height / Rows;

	// TODO create the cells
	float startX = -Width / 2; // 0;
	float startY = -Height / 2; //0;
	CellOrigin = FVector2D(startX, startY);
	
	for (int y = 0; y < NrOfRows; ++y)
	{
		for (int x = 0; x < NrOfCols; ++x)
		{
			float left = startX + x * CellWidth;
			float bot = startY + y * CellHeight;

			Cells.push_back(Cell(left, bot, CellWidth, CellHeight));
		}
	}
}

void CellSpace::AddAgent(ASteeringAgent& Agent)
{
	// TODO Add the agent to the correct cell
	int idx = PositionToIndex(Agent.GetPosition());

	Cells[idx].Agents.push_back(&Agent);
}

void CellSpace::UpdateAgentCell(ASteeringAgent& Agent, const FVector2D& OldPos)
{
	//TODO Check if the agent needs to be moved to another cell.
	//TODO Use the calculated index for oldPos and currentPos for this
	int OldIdx = PositionToIndex(OldPos);
	int NewIdx = PositionToIndex(Agent.GetPosition());

	if (NewIdx == OldIdx)
		return;

	// if the index has changed, remove the agent from the old cell and add it to the new one
	Cells[OldIdx].Agents.remove(&Agent);
	Cells[NewIdx].Agents.push_back(&Agent);
}

void CellSpace::RegisterNeighbors(ASteeringAgent& Agent, float QueryRadius)
{
	// TODO Register the neighbors for the provided agent
	// TODO Only check the cells that are within the radius of the neighborhood
	NrOfNeighbors = 0;
	const FVector2D Pos = Agent.GetPosition();

	// create the query box that is the bounding box of the target's query circle
	FRect QueryBox{
			{Pos.X - QueryRadius, Pos.Y - QueryRadius},
			{Pos.X + (QueryRadius * 2), Pos.Y + (QueryRadius * 2)}
	};
	
	// iterate through each cell and test to see if its bounding box overlaps
	// with the query box. If it does and it also contains entities then
	// make further proximity tests.
	const int MaxNeighbors = Neighbors.Num();

	// std::vector<Cell>::iterator currentCell; // TODO FIXME this was already in but never used and broken/shadowing
	int NrOfCells{ 0 };
	float QueryRadiusSquared = FMath::Pow(QueryRadius, 2);
	for (const Cell& CurrentCell : Cells)
	{
		//test to see if this cell contains members and if it overlaps the
		//query box
		if (DoRectsOverlap(CurrentCell.BoundingBox, QueryBox) && !CurrentCell.Agents.empty())
		{
			++NrOfCells;
			//add any entities found within query radius to the neighbor list
			for (ASteeringAgent* const CurrentAgent : CurrentCell.Agents)
			{
				if (CurrentAgent == &Agent)
					continue;

				FVector2D fromNeighbor = Pos - CurrentAgent->GetPosition();
				float distanceSquared = fromNeighbor.SquaredLength();
				if (distanceSquared < QueryRadiusSquared)
				{
					if (NrOfNeighbors < MaxNeighbors)
					{
						Neighbors[NrOfNeighbors] = CurrentAgent;
						++NrOfNeighbors;
					}
				}
			}
		}
	}//next cell
}

void CellSpace::EmptyCells()
{
	for (Cell& c : Cells)
		c.Agents.clear();
}

void CellSpace::RenderCells() const
{
	// TODO Render the cells with the number of agents inside of it
	for (const Cell& CurrentCell : Cells)
	{
		FVector2D ToCenter = FVector2D{CurrentCell.BoundingBox.Max - CurrentCell.BoundingBox.Min} / 2.0f;
		float Width = (CurrentCell.BoundingBox.Max.X - CurrentCell.BoundingBox.Min.X) / 2;
		float Depth = (CurrentCell.BoundingBox.Max.Y - CurrentCell.BoundingBox.Min.Y) / 2;
		
		FVector BoxCenter{CurrentCell.BoundingBox.Min + ToCenter, 1.f};
		FVector BoxExtents{Width, Depth, 1.0f};
		DrawDebugBox(pWorld, BoxCenter, BoxExtents, FColorList::Blue);
		
		FString Text = FString::Printf(TEXT("%d"), static_cast<int>(CurrentCell.Agents.size()));
		DrawDebugString(pWorld, BoxCenter, FString(Text), 0, FColor::Blue, 0);
	}
}

int CellSpace::PositionToIndex(FVector2D const & Pos) const
{
	// TODO Calculate the index of the cell based on the position
	FVector2D ToAgent{Pos - CellOrigin};
	int col = ToAgent.X / CellWidth;
	int row = ToAgent.Y / CellHeight;
	int idx = row * NrOfCols + col;

	if (idx >= static_cast<int>(Cells.size()))
		idx = static_cast<int>(Cells.size()) - 1;

	return idx;
}

bool CellSpace::DoRectsOverlap(FRect const & RectA, FRect const & RectB)
{
	// Check if the rectangles are separated on either axis
	if (RectA.Max.X < RectB.Min.X || RectA.Min.X > RectB.Max.X) return false;
	if (RectA.Max.Y < RectB.Min.Y || RectA.Min.Y > RectB.Max.Y) return false;
    
	// If they are not separated, they must overlap
	return true;
}