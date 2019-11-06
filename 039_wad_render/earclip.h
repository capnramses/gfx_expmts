// WAD Rend - Copyright 2017 Anton Gerdelan <antonofnote@gmail.com>
// C99
/* Notes:
For polygon P with V vertices and E edges, and number of triangulated faces F:
* χ = V - E + F Euler characteristic (lower-case chi) 
* V - E + F = 2 for convex polyhedra and other numbers for non convex
* 2E - V = 3F from Dehn–Sommerville equations so
* F = V - 2

* edges on the outside 'edges'. on the inside = 'diagonals'
* diagonals connect 2 adjacent verts and DO NOT CROSS any edges
* idea is to find any first diagonal, split the polygon based on this, then recurse

* we are forming a BINARY TREE of triangles
* this is clear if we draw the graph of centre triangle-> centretriangle
  - at most 3 connections
  - removing any vertex disconnects the graph
  - Catalan number Cn defines number of connections
  - no cycles or cross-over loops allowed

* the simplest method that can also be extended to deal with holes and non-convex shapes
  is the 'ear clipping' algorithm. Two Ear Theorum - every simple polygon has two 'ears'
  - it's O(n^2) + extras -> O(n^3) time
  - "while P is not a triangle - find and cut ear"
  - it's possible to improve this and do more splitting in the loop to approach O(n^2)
*/
#pragma once
