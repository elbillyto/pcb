/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

static char *rcsid = "$Id$";

/* functions used to move pins, elements ...
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "global.h"

#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "misc.h"
#include "move.h"
#include "mymem.h"
#include "polygon.h"
#include "search.h"
#include "select.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *MoveElementName (ElementTypePtr);
static void *MoveElement (ElementTypePtr);
static void *MoveVia (PinTypePtr);
static void *MoveLine (LayerTypePtr, LineTypePtr);
static void *MoveArc (LayerTypePtr, ArcTypePtr);
static void *MoveText (LayerTypePtr, TextTypePtr);
static void *MovePolygon (LayerTypePtr, PolygonTypePtr);
static void *MoveLinePoint (LayerTypePtr, LineTypePtr, PointTypePtr);
static void *MovePolygonPoint (LayerTypePtr, PolygonTypePtr, PointTypePtr);
static void *MoveLineToLayer (LayerTypePtr, LineTypePtr);
static void *MoveArcToLayer (LayerTypePtr, ArcTypePtr);
static void *MoveRatToLayer (RatTypePtr);
static void *MoveTextToLayer (LayerTypePtr, TextTypePtr);
static void *MovePolygonToLayer (LayerTypePtr, PolygonTypePtr);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Location DeltaX,		/* used by local routines as offset */
  DeltaY;
static LayerTypePtr Dest;
static Boolean MoreToCome;
static ObjectFunctionType MoveFunctions = {
  MoveLine,
  MoveText,
  MovePolygon,
  MoveVia,
  MoveElement,
  MoveElementName,
  NULL,
  NULL,
  MoveLinePoint,
  MovePolygonPoint,
  MoveArc,
  NULL
}, MoveToLayerFunctions =

{
MoveLineToLayer,
    MoveTextToLayer,
    MovePolygonToLayer,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, MoveArcToLayer, MoveRatToLayer};

/* ---------------------------------------------------------------------------
 * moves a element by +-X and +-Y
 */
void
MoveElementLowLevel (ElementTypePtr Element, Location DX, Location DY)
{
  ELEMENTLINE_LOOP (Element, 
    {
      MOVE_LINE_LOWLEVEL (line, DX, DY);
    }
  );
  PIN_LOOP (Element, 
    {
      MOVE_PIN_LOWLEVEL (pin, DX, DY);
    }
  );
  PAD_LOOP (Element, 
    {
      MOVE_PAD_LOWLEVEL (pad, DX, DY);
    }
  );
  ARC_LOOP (Element, 
    {
      MOVE_ARC_LOWLEVEL (arc, DX, DY);
    }
  );
  ELEMENTTEXT_LOOP (Element, 
    {
      MOVE_TEXT_LOWLEVEL (text, DX, DY);
    }
  );
  MOVE_BOX_LOWLEVEL (&Element->BoundingBox, DX, DY);
  MOVE (Element->MarkX, Element->MarkY, DX, DY);
}

/* ----------------------------------------------------------------------
 * moves all names of an element to a new position
 */
static void *
MoveElementName (ElementTypePtr Element)
{
  if (PCB->ElementOn && (FRONT (Element) || PCB->InvisibleObjectsOn))
    {
      EraseElementName (Element);
      ELEMENTTEXT_LOOP (Element, 
	{
	  MOVE_TEXT_LOWLEVEL (text, DeltaX, DeltaY);
	}
      );
      DrawElementName (Element, 0);
      Draw ();
    }
  else
    {
      ELEMENTTEXT_LOOP (Element, 
	{
	  MOVE_TEXT_LOWLEVEL (text, DeltaX, DeltaY);
	}
      );
    }
  return (Element);
}

/* ---------------------------------------------------------------------------
 * moves an element
 */
static void *
MoveElement (ElementTypePtr Element)
{
  Boolean didDraw = False;

  if (PCB->ElementOn && (FRONT (Element) || PCB->InvisibleObjectsOn))
    {
      EraseElement (Element);
      MoveElementLowLevel (Element, DeltaX, DeltaY);
      DrawElementName (Element, 0);
      DrawElementPackage (Element, 0);
      didDraw = True;
    }
  else
    {
      if (PCB->PinOn)
	EraseElementPinsAndPads (Element);
      MoveElementLowLevel (Element, DeltaX, DeltaY);
    }
  UpdatePIPFlags (NULL, Element, NULL, NULL, True);
  if (PCB->PinOn)
    {
      DrawElementPinsAndPads (Element, 0);
      didDraw = True;
    }
  if (didDraw)
    Draw ();
  return (Element);
}

/* ---------------------------------------------------------------------------
 * moves a via
 */
static void *
MoveVia (PinTypePtr Via)
{
  if (PCB->ViaOn)
    {
      EraseVia (Via);
      MOVE_VIA_LOWLEVEL (Via, DeltaX, DeltaY);
    }
  else
    MOVE_VIA_LOWLEVEL (Via, DeltaX, DeltaY);
  UpdatePIPFlags (Via, (ElementTypePtr) Via, NULL, NULL, True);
  if (PCB->ViaOn)
    {
      DrawVia (Via, 0);
      Draw ();
    }
  return (Via);
}

/* ---------------------------------------------------------------------------
 * moves a line
 */
static void *
MoveLine (LayerTypePtr Layer, LineTypePtr Line)
{
  if (Layer->On)
    {
      EraseLine (Line);
      MOVE_LINE_LOWLEVEL (Line, DeltaX, DeltaY);
      DrawLine (Layer, Line, 0);
      Draw ();
    }
  else
    MOVE_LINE_LOWLEVEL (Line, DeltaX, DeltaY);
  return (Line);
}

/* ---------------------------------------------------------------------------
 * moves an arc
 */
static void *
MoveArc (LayerTypePtr Layer, ArcTypePtr Arc)
{
  if (Layer->On)
    {
      EraseArc (Arc);
      MOVE_ARC_LOWLEVEL (Arc, DeltaX, DeltaY);
      SetArcBoundingBox (Arc);
      DrawArc (Layer, Arc, 0);
      Draw ();
    }
  else
    {
      MOVE_ARC_LOWLEVEL (Arc, DeltaX, DeltaY);
      SetArcBoundingBox (Arc);
    }
  return (Arc);
}

/* ---------------------------------------------------------------------------
 * moves a text object
 */
static void *
MoveText (LayerTypePtr Layer, TextTypePtr Text)
{
  if (Layer->On)
    {
      EraseText (Text);
      MOVE_TEXT_LOWLEVEL (Text, DeltaX, DeltaY);
      DrawText (Layer, Text, 0);
      Draw ();
    }
  else
    MOVE_TEXT_LOWLEVEL (Text, DeltaX, DeltaY);
  return (Text);
}

/* ---------------------------------------------------------------------------
 * low level routine to move a polygon
 */
void
MovePolygonLowLevel (PolygonTypePtr Polygon, Location DeltaX, Location DeltaY)
{
  POLYGONPOINT_LOOP (Polygon, 
    {
      MOVE (point->X, point->Y, DeltaX, DeltaY);
    }
  );
  MOVE_BOX_LOWLEVEL (&Polygon->BoundingBox, DeltaX, DeltaY);
}

/* ---------------------------------------------------------------------------
 * moves a polygon
 */
static void *
MovePolygon (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  if (Layer->On)
    {
      ErasePolygon (Polygon);
      MovePolygonLowLevel (Polygon, DeltaX, DeltaY);
      DrawPolygon (Layer, Polygon, 0);
      Draw ();
    }
  else
    MovePolygonLowLevel (Polygon, DeltaX, DeltaY);
  UpdatePIPFlags (NULL, NULL, Layer, Polygon, True);
  return (Polygon);
}

/* ---------------------------------------------------------------------------
 * moves one end of a line
 */
static void *
MoveLinePoint (LayerTypePtr Layer, LineTypePtr Line, PointTypePtr Point)
{
  if (Layer)
    {
      if (Layer->On)
	{
	  EraseLine (Line);
	  MOVE (Point->X, Point->Y, DeltaX, DeltaY) DrawLine (Layer, Line, 0);
	  Draw ();
	}
      else
	MOVE (Point->X, Point->Y, DeltaX, DeltaY) return (Line);
    }
  else				/* must be a rat */
    {
      if (PCB->RatOn)
	{
	  EraseRat ((RatTypePtr) Line);
	  MOVE (Point->X, Point->Y, DeltaX, DeltaY)
	    DrawRat ((RatTypePtr) Line, 0);
	  Draw ();
	}
      else
	MOVE (Point->X, Point->Y, DeltaX, DeltaY) return (Line);
    }
}

/* ---------------------------------------------------------------------------
 * moves a polygon-point
 */
static void *
MovePolygonPoint (LayerTypePtr Layer, PolygonTypePtr Polygon,
		  PointTypePtr Point)
{
  if (Layer->On)
    {
      ErasePolygon (Polygon);
      MOVE (Point->X, Point->Y, DeltaX, DeltaY);
      if (!RemoveExcessPolygonPoints (Layer, Polygon))
	{
	  SetPolygonBoundingBox (Polygon);
	  DrawPolygon (Layer, Polygon, 0);
	  Draw ();
	}
    }
  else
    {
      MOVE (Point->X, Point->Y, DeltaX, DeltaY);
      if (!RemoveExcessPolygonPoints (Layer, Polygon))
	SetPolygonBoundingBox (Polygon);
    }
  UpdatePIPFlags (NULL, NULL, Layer, Polygon, True);
  return (Point);
}

/* ---------------------------------------------------------------------------
 * moves a line between layers; lowlevel routines
 */
void *
MoveLineToLayerLowLevel (LayerTypePtr Source, LineTypePtr Line,
			 LayerTypePtr Destination)
{
  LineTypePtr new = GetLineMemory (Destination);

  /* copy the data and remove it from the former layer */
  *new = *Line;
  *Line = Source->Line[--Source->LineN];
  memset (&Source->Line[Source->LineN], 0, sizeof (LineType));
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves an arc between layers; lowlevel routines
 */
void *
MoveArcToLayerLowLevel (LayerTypePtr Source, ArcTypePtr Arc,
			LayerTypePtr Destination)
{
  ArcTypePtr new = GetArcMemory (Destination);

  /* copy the data and remove it from the former layer */
  *new = *Arc;
  *Arc = Source->Arc[--Source->ArcN];
  memset (&Source->Arc[Source->ArcN], 0, sizeof (ArcType));
  return (new);
}


/* ---------------------------------------------------------------------------
 * moves an arc between layers
 */
static void *
MoveArcToLayer (LayerTypePtr Layer, ArcTypePtr Arc)
{
  ArcTypePtr new;

  if (Dest == Layer && Layer->On)
    {
      DrawArc (Layer, Arc, 0);
      Draw ();
    }
  if (((long int) Dest == -1) || Dest == Layer)
    return (Arc);
  AddObjectToMoveToLayerUndoList (ARC_TYPE, Layer, Arc, Arc);
  if (Layer->On)
    EraseArc (Arc);
  new = MoveArcToLayerLowLevel (Layer, Arc, Dest);
  if (Dest->On)
    DrawArc (Dest, new, 0);
  Draw ();
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves a line between layers
 */
static void *
MoveRatToLayer (RatTypePtr Rat)
{
  LineTypePtr new;

  new = CreateNewLineOnLayer (Dest, Rat->Point1.X, Rat->Point1.Y,
			      Rat->Point2.X, Rat->Point2.Y,
			      Settings.LineThickness, Settings.Keepaway,
			      (Rat->Flags & ~RATFLAG) |
			      (TEST_FLAG (CLEARNEWFLAG, PCB) ? CLEARLINEFLAG :
			       0));
  if (!new)
    return (NULL);
  AddObjectToCreateUndoList (LINE_TYPE, Dest, new, new);
  if (PCB->RatOn)
    EraseRat (Rat);
  MoveObjectToRemoveUndoList (RATLINE_TYPE, Rat, Rat, Rat);
  DrawLine (Dest, new, 0);
  Draw ();
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves a line between layers
 */
static void *
MoveLineToLayer (LayerTypePtr Layer, LineTypePtr Line)
{
  LineTypePtr new;
  PinTypePtr via;
  void *ptr1, *ptr2, *ptr3;

  if (Dest == Layer && Layer->On)
    {
      DrawLine (Layer, Line, 0);
      Draw ();
    }
  if (((long int) Dest == -1) || Dest == Layer)
    return (Line);

  AddObjectToMoveToLayerUndoList (LINE_TYPE, Layer, Line, Line);
  if (Layer->On)
    EraseLine (Line);
  new = MoveLineToLayerLowLevel (Layer, Line, Dest);
  if (Dest->On)
    DrawLine (Dest, new, 0);
  Draw ();
  if (!PCB->ViaOn || MoreToCome ||
      GetLayerGroupNumberByPointer (Layer) ==
      GetLayerGroupNumberByPointer (Dest))
    return (new);
  if ((SearchObjectByLocation (PIN_TYPES, &ptr1, &ptr2, &ptr3,
			       new->Point1.X, new->Point1.Y, 0) == NO_TYPE))
    LINE_LOOP (Layer, 
    {
      if (((line->Point1.X == new->Point1.X)
	   && (line->Point1.Y == new->Point1.Y))
	  || ((line->Point2.X == new->Point1.X)
	      && (line->Point2.Y == new->Point1.Y)))
	{
	  if ((via =
	       CreateNewVia (PCB->Data, new->Point1.X, new->Point1.Y,
			     Settings.ViaThickness, 2 * Settings.Keepaway,
			     0, Settings.ViaDrillingHole, NULL,
			     VIAFLAG)) != NULL)
	    {
	      UpdatePIPFlags (via, (ElementTypePtr) via, NULL, NULL, False);
	      AddObjectToCreateUndoList (VIA_TYPE, via, via, via);
	      DrawVia (via, 0);
	    }
	  break;
	}
    }
  );
  if ((SearchObjectByLocation (PIN_TYPES, &ptr1, &ptr2, &ptr3,
			       new->Point2.X, new->Point2.Y, 0) == NO_TYPE))
    LINE_LOOP (Layer, 
    {
      if (((line->Point1.X == new->Point2.X)
	   && (line->Point1.Y == new->Point2.Y))
	  || ((line->Point2.X == new->Point2.X)
	      && (line->Point2.Y == new->Point2.Y)))
	{
	  if ((via =
	       CreateNewVia (PCB->Data, new->Point2.X, new->Point2.Y,
			     Settings.ViaThickness, 2 * Settings.Keepaway,
			     0, Settings.ViaDrillingHole, NULL,
			     VIAFLAG)) != NULL)
	    {
	      UpdatePIPFlags (via, (ElementTypePtr) via, NULL, NULL, False);
	      AddObjectToCreateUndoList (VIA_TYPE, via, via, via);
	      DrawVia (via, 0);
	    }
	  break;
	}
    }
  );
  Draw ();
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves a text object between layers; lowlevel routines
 */
void *
MoveTextToLayerLowLevel (LayerTypePtr Source, TextTypePtr Text,
			 LayerTypePtr Destination)
{
  TextTypePtr new = GetTextMemory (Destination);

  /* copy the data and remove it from the former layer */
  *new = *Text;
  *Text = Source->Text[--Source->TextN];
  memset (&Source->Text[Source->TextN], 0, sizeof (TextType));
  if (GetLayerGroupNumberByNumber (MAX_LAYER + SOLDER_LAYER) ==
      GetLayerGroupNumberByPointer (Destination))
    SET_FLAG (ONSOLDERFLAG, new);
  else
    CLEAR_FLAG (ONSOLDERFLAG, new);
  /* re-calculate the bounding box (it could be mirrored now) */
  SetTextBoundingBox (&PCB->Font, new);
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves a text object between layers
 */
static void *
MoveTextToLayer (LayerTypePtr Layer, TextTypePtr Text)
{
  TextTypePtr new;

  if (Dest != Layer)
    {
      AddObjectToMoveToLayerUndoList (TEXT_TYPE, Layer, Text, Text);
      new = MoveTextToLayerLowLevel (Layer, Text, Dest);
      if (Layer->On)
	EraseText (Text);
      if (Dest->On)
	DrawText (Dest, new, 0);
      if (Layer->On || Dest->On)
	Draw ();
      return (new);
    }
  return (Text);
}

/* ---------------------------------------------------------------------------
 * moves a polygon between layers; lowlevel routines
 */
void *
MovePolygonToLayerLowLevel (LayerTypePtr Source, PolygonTypePtr Polygon,
			    LayerTypePtr Destination)
{
  PolygonTypePtr new = GetPolygonMemory (Destination);

  /* copy the data and remove it from the former layer */
  *new = *Polygon;
  *Polygon = Source->Polygon[--Source->PolygonN];
  memset (&Source->Polygon[Source->PolygonN], 0, sizeof (PolygonType));
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves a polygon between layers
 */
static void *
MovePolygonToLayer (LayerTypePtr Layer, PolygonTypePtr Polygon)
{
  PolygonTypePtr new;
  int LayerThermFlag, DestThermFlag;

  if (((long int) Dest == -1) || (Layer == Dest))
    return (Polygon);
  AddObjectToMoveToLayerUndoList (POLYGON_TYPE, Layer, Polygon, Polygon);
  if (Layer->On)
    ErasePolygon (Polygon);
  /* Move all of the thermals with the polygon */
  LayerThermFlag = L0THERMFLAG << GetLayerNumber (PCB->Data, Layer);
  DestThermFlag = L0THERMFLAG << GetLayerNumber (PCB->Data, Dest);
  ALLPIN_LOOP (PCB->Data, 
    {
      if (TEST_FLAG (LayerThermFlag, pin) &&
	  IsPointInPolygon (pin->X, pin->Y, 0, Polygon))
	{
	  AddObjectToFlagUndoList (PIN_TYPE, Layer, pin, pin);
	  CLEAR_FLAG (LayerThermFlag, pin);
	  SET_FLAG (DestThermFlag, pin);
	}
    }
  );
  VIA_LOOP (PCB->Data, 
    {
      if (TEST_FLAG (LayerThermFlag, via) &&
	  IsPointInPolygon (via->X, via->Y, 0, Polygon))
	{
	  AddObjectToFlagUndoList (VIA_TYPE, Layer, via, via);
	  CLEAR_FLAG (LayerThermFlag, via);
	  SET_FLAG (DestThermFlag, via);
	}
    }
  );
  new = MovePolygonToLayerLowLevel (Layer, Polygon, Dest);
  UpdatePIPFlags (NULL, NULL, Layer, NULL, True);
  UpdatePIPFlags (NULL, NULL, Dest, new, True);
  if (Dest->On)
    {
      DrawPolygon (Dest, new, 0);
      Draw ();
    }
  return (new);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * not we don't bump the undo serial number
 */
void *
MoveObject (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
	    Location DX, Location DY)
{
  void *result;
  /* setup offset */
  DeltaX = DX;
  DeltaY = DY;
  AddObjectToMoveUndoList (Type, Ptr1, Ptr2, Ptr3, DX, DY);
  result = ObjectOperation (&MoveFunctions, Type, Ptr1, Ptr2, Ptr3);
  return (result);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * as well as all attached rubberband lines
 */
void *
MoveObjectAndRubberband (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
			 Location DX, Location DY)
{
  RubberbandTypePtr ptr;
  void *ptr2;

  /* setup offset */
  DeltaX = DX;
  DeltaY = DY;
  if (DX == 0 && DY == 0)
    return (NULL);

  /* move all the lines... and reset the counter */
  ptr = Crosshair.AttachedObject.Rubberband;
  while (Crosshair.AttachedObject.RubberbandN)
    {
      /* first clear any marks that we made in the line flags */
      ptr->Line->Flags &= ~RUBBERENDFLAG;
      AddObjectToMoveUndoList (LINEPOINT_TYPE,
			       ptr->Layer, ptr->Line, ptr->MovedPoint, DX,
			       DY);
      MoveLinePoint (ptr->Layer, ptr->Line, ptr->MovedPoint);
      Crosshair.AttachedObject.RubberbandN--;
      ptr++;
    }

  AddObjectToMoveUndoList (Type, Ptr1, Ptr2, Ptr3, DX, DY);
  ptr2 = ObjectOperation (&MoveFunctions, Type, Ptr1, Ptr2, Ptr3);
  IncrementUndoSerialNumber ();
  return (ptr2);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * to a new layer without changing it's position
 */
void *
MoveObjectToLayer (int Type, void *Ptr1, void *Ptr2, void *Ptr3,
		   LayerTypePtr Target, Boolean enmasse)
{
  void *result;

  /* setup global identifiers */
  Dest = Target;
  MoreToCome = enmasse;
  result = ObjectOperation (&MoveToLayerFunctions, Type, Ptr1, Ptr2, Ptr3);
  IncrementUndoSerialNumber ();
  return (result);
}

/* ---------------------------------------------------------------------------
 * moves the selected objects to a new layer without changing their
 * positions
 */
Boolean
MoveSelectedObjectsToLayer (LayerTypePtr Target)
{
  Boolean changed;

  /* setup global identifiers */
  Dest = Target;
  MoreToCome = True;
  changed = SelectedOperation (&MoveToLayerFunctions, True, ALL_TYPES);
  /* passing True to above operation causes Undoserial to auto-increment */
  return (changed);
}
