/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef BACKENDS_GRAPHICS_SPLIT_SCREEN_LAYOUT_H
#define BACKENDS_GRAPHICS_SPLIT_SCREEN_LAYOUT_H

#include "common/algorithm.h"
#include "common/array.h"
#include "common/rect.h"
#include "common/system.h"

/**
 * Lays out the interface text of a game (sentence line, verbs, inventory)
 * as large touch friendly rows in the bottom area of a split screen.
 *
 * Elements keep the relative column and row order they have in the game,
 * but each one is scaled on its own and given room around it.
 */
class SplitScreenLayout {
public:
	typedef OSystem::SplitScreenElement Element;

	struct Cell {
		Common::Rect src;  ///< Where the element is in the game screen
		Common::Rect draw; ///< Where it is drawn in the window
		Common::Rect hit;  ///< Where a tap selects it (empty if not selectable)
		bool linear;       ///< Taps map proportionally over the whole of src, not to its center
	};

	void setElements(const Common::Array<Element> &elements) {
		_elements = elements;
		buildBlocks();
	}

	const Common::Array<Cell> &cells() const { return _cells; }

	void compute(const Common::Rect &area, int gameWidth) {
		_cells.clear();
		_area = area;
		_gameWidth = gameWidth;
		if (area.isEmpty())
			return;

		const int areaW = area.width();
		const int areaH = area.height();

		if (_regionMode) {
			computeRegions(area);
			return;
		}

		// The rows are spread over the whole height, with half a row between blocks
		const int rows = (_hasSentence ? 1 : 0) +
		                 MAX(_blocks[0].rows, _blocks[0].minRows) +
		                 MAX(_blocks[1].rows, _blocks[1].minRows);
		const float pitch = areaH / (rows + 1.0f);
		float y = area.top;

		if (_hasSentence) {
			const float scale = MIN<float>((float)areaW / _gameWidth, pitch / kRowUnits);
			for (uint i = 0; i < _elements.size(); i++) {
				if (_elements[i].group != 0)
					continue;
				Cell c;
				c.linear = false;
				c.src = _elements[i].src;
				const int h = MAX<int>(c.src.height(), kTextHeight);
				const int dw = (int)(c.src.width() * scale);
				const int dh = (int)(h * scale);
				const int dx = area.left + (int)(c.src.left * scale);
				const int dy = (int)(y + (pitch - dh) / 2);
				c.draw = Common::Rect(dx, dy, dx + MAX(dw, 1), dy + MAX(dh, 1));
				_cells.push_back(c);
			}
			y += pitch;
		}

		for (int b = 0; b < 2; b++) {
			const Block &block = _blocks[b];
			y += pitch * 0.5f;

			float widthU = kPadding;
			for (uint c = 0; c < block.colW.size(); c++)
				widthU += block.colW[c] + (c ? kGap : 0);
			widthU = MAX<float>(widthU, block.minWidth);

			const float scale = MIN<float>(areaW / widthU, pitch / kRowUnits);
			float x = area.left + (areaW - widthU * scale) / 2 + kPadding / 2 * scale;
			Common::Array<float> colX;
			for (uint c = 0; c < block.colW.size(); c++) {
				colX.push_back(x);
				x += (block.colW[c] + kGap) * scale;
			}

			for (uint i = 0; i < block.idx.size(); i++) {
				const Element &e = _elements[block.idx[i]];
				const int col = block.colOf[i];
				const int row = block.rowOf[i];
				const float rowY = y + row * pitch;

				Cell c;
				c.linear = false;
				c.src = e.src;
				const int h = MAX<int>(e.src.height(), kTextHeight);
				const int dw = (int)(e.src.width() * scale);
				const int dh = (int)(h * scale);
				const int dx = (int)colX[col];
				const int dy = (int)(rowY + (pitch - dh) / 2);
				c.draw = Common::Rect(dx, dy, dx + MAX(dw, 1), dy + MAX(dh, 1));

				const int hitL = (int)(colX[col] - kGap / 2 * scale);
				const int hitR = (int)(colX[col] + (block.colW[col] + kGap / 2) * scale);
				c.hit = Common::Rect(MAX<int>(hitL, area.left), (int)rowY,
				                     MIN<int>(hitR, area.right), (int)(rowY + pitch));
				_cells.push_back(c);
			}
			y += MAX(block.rows, block.minRows) * pitch;
		}
	}

	/** Finds the game screen position to report for a tap in the window. */
	Common::Point windowToGame(int x, int y, int gameWidth, int gameHeight) const {
		for (uint i = 0; i < _cells.size(); i++) {
			const Cell &c = _cells[i];
			if (c.hit.isEmpty() || !c.hit.contains(x, y))
				continue;
			if (c.linear)
				return Common::Point(CLIP<int>(c.src.left + (x - c.hit.left) * c.src.width() / c.hit.width(), c.src.left, c.src.right - 1),
				                     CLIP<int>(c.src.top + (y - c.hit.top) * c.src.height() / c.hit.height(), c.src.top, c.src.bottom - 1));
			return Common::Point(c.src.left + c.src.width() / 2, c.src.top + c.src.height() / 2);
		}
		// Nothing there: the bottom corner of the game screen is inert
		return Common::Point(gameWidth - 1, gameHeight - 1);
	}

	/** Finds the window position of what is at a game screen position. */
	Common::Point gameToWindow(int x, int y) const {
		for (uint i = 0; i < _cells.size(); i++) {
			const Cell &c = _cells[i];
			if (c.hit.isEmpty() || !c.src.contains(x, y))
				continue;
			if (c.linear)
				return Common::Point(c.hit.left + (x - c.src.left) * c.hit.width() / c.src.width(),
				                     c.hit.top + (y - c.src.top) * c.hit.height() / c.src.height());
			return Common::Point(c.hit.left + c.hit.width() / 2, c.hit.top + c.hit.height() / 2);
		}
		return Common::Point(_area.right - 1, _area.bottom - 1);
	}

private:
	enum {
		kTextHeight = 8,
		kRowUnits = 12, // Text height plus spacing, in game pixels
		kGap = 10,      // Between columns, in game pixels
		kPadding = 4,
		kMaxColumns = 4
	};

	struct Block {
		Common::Array<int> idx;    // Indices in _elements
		Common::Array<int> colOf;  // Per element
		Common::Array<int> rowOf;  // Per element
		Common::Array<int> colW;   // Per column, in game pixels
		int rows = 0;
		int minRows = 0;           // Rows kept free so the layout doesn't jump around
		int minWidth = 0;          // Same for the width
	};

	static void cluster(const Common::Array<int> &values, int tolerance, Common::Array<int> &reps) {
		Common::Array<int> sorted(values);
		Common::sort(sorted.begin(), sorted.end());
		reps.clear();
		for (uint i = 0; i < sorted.size(); i++) {
			if (reps.empty() || sorted[i] - reps.back() > tolerance)
				reps.push_back(sorted[i]);
		}
	}

	static int indexOf(const Common::Array<int> &reps, int value, int tolerance) {
		for (uint i = reps.size(); i > 0; i--) {
			if (value >= reps[i - 1] && value - reps[i - 1] <= tolerance)
				return i - 1;
		}
		return 0;
	}

	void buildBlocks() {
		_hasSentence = false;
		for (int b = 0; b < 2; b++)
			_blocks[b] = Block();
		_blocks[0].minRows = 4;
		_blocks[0].minWidth = 190;
		_blocks[1].minRows = 2;
		_blocks[1].minWidth = 200;

		_regionMode = false;
		for (uint i = 0; i < _elements.size(); i++) {
			const uint8 g = _elements[i].group;
			if (g >= 4)
				_regionMode = true;
			else if (g == 0)
				_hasSentence = true;
			else
				_blocks[g == 1 ? 0 : 1].idx.push_back(i);
		}

		for (int b = 0; b < 2; b++) {
			Block &block = _blocks[b];
			Common::Array<int> lefts, tops, colReps, rowReps;
			for (uint i = 0; i < block.idx.size(); i++) {
				lefts.push_back(_elements[block.idx[i]].src.left);
				tops.push_back(_elements[block.idx[i]].src.top);
			}
			cluster(lefts, 7, colReps);
			cluster(tops, 3, rowReps);
			block.rows = rowReps.size();
			int cols = colReps.size();
			for (uint i = 0; i < block.idx.size(); i++) {
				const Element &e = _elements[block.idx[i]];
				block.colOf.push_back(indexOf(colReps, e.src.left, 7));
				block.rowOf.push_back(indexOf(rowReps, e.src.top, 3));
			}

			// With more columns than fit, the verbs of the extra ones go on a row of their own
			if (b == 0 && cols > kMaxColumns) {
				int next = 0;
				for (int c = kMaxColumns; c < cols; c++) {
					for (int r = 0; r < block.rows; r++) {
						for (uint i = 0; i < block.idx.size(); i++) {
							if (block.colOf[i] == c && block.rowOf[i] == r) {
								block.colOf[i] = -1 - next;
								block.rowOf[i] = block.rows;
								next++;
							}
						}
					}
				}
				for (uint i = 0; i < block.idx.size(); i++) {
					if (block.colOf[i] < 0)
						block.colOf[i] = -1 - block.colOf[i];
				}
				block.rows++;
				cols = kMaxColumns;
			}

			block.colW.resize(cols);
			for (int c = 0; c < cols; c++)
				block.colW[c] = 0;
			for (uint i = 0; i < block.idx.size(); i++) {
				const int col = block.colOf[i];
				block.colW[col] = MAX<int>(block.colW[col], _elements[block.idx[i]].src.width());
			}
		}
	}

	static void unite(Common::Rect &r, bool &has, const Common::Rect &other) {
		if (has)
			r.extend(other);
		else
			r = other;
		has = true;
	}

	void addCell(const Common::Rect &src, const Common::Rect &draw, const Common::Rect &hit, bool linear) {
		Cell c;
		c.linear = linear;
		c.src = src;
		c.draw = draw;
		c.hit = hit;
		_cells.push_back(c);
	}

	// Redraws the box holding the verb texts so that its columns are as wide as
	// the area and equally wide, keeping the texts centered in them at scale sy.
	bool addStretchedBox(const Common::Rect &box, const Common::Array<Common::Rect> &texts,
	                     const Common::Rect &area, float sy, float top, int fillX = -1) {
		Common::Array<int> centersX, centersY, colReps, rowReps;
		for (uint i = 0; i < texts.size(); i++) {
			centersX.push_back((texts[i].left + texts[i].right) / 2);
			centersY.push_back((texts[i].top + texts[i].bottom) / 2);
		}
		cluster(centersX, 6, colReps);
		cluster(centersY, 3, rowReps);
		const int nCols = colReps.size(), nRows = rowReps.size();
		if (nCols == 0 || nRows == 0)
			return false;

		Common::Array<float> colCenter(nCols), rowCenter(nRows);
		Common::Array<int> colCount(nCols), rowCount(nRows), spanA(nCols), spanB(nCols);
		for (int c = 0; c < nCols; c++) {
			colCenter[c] = 0; colCount[c] = 0; spanA[c] = 9999; spanB[c] = -9999;
		}
		for (int r = 0; r < nRows; r++) {
			rowCenter[r] = 0; rowCount[r] = 0;
		}
		for (uint i = 0; i < texts.size(); i++) {
			const int c = indexOf(colReps, centersX[i], 6);
			const int r = indexOf(rowReps, centersY[i], 3);
			colCenter[c] += centersX[i]; colCount[c]++;
			rowCenter[r] += centersY[i]; rowCount[r]++;
			spanA[c] = MIN<int>(spanA[c], texts[i].left);
			spanB[c] = MAX<int>(spanB[c], texts[i].right);
		}
		for (int c = 0; c < nCols; c++)
			colCenter[c] /= colCount[c];
		for (int r = 0; r < nRows; r++)
			rowCenter[r] /= rowCount[r];

		const float cw = nCols > 1 ? (colCenter[nCols - 1] - colCenter[0]) / (nCols - 1) : (float)box.width() - 8;
		const float ph = nRows > 1 ? (rowCenter[nRows - 1] - rowCenter[0]) / (nRows - 1) : box.height() / 3.0f;
		const int left0 = MAX<int>((int)(colCenter[0] - cw / 2 + 0.5f), box.left);
		const int rightN = MIN<int>((int)(colCenter[nCols - 1] + cw / 2 + 0.5f), box.right);
		if (cw < 8 || left0 > box.left + 16 || rightN < box.right - 16)
			return false;

		// Without a frame, the buttons are not known to be equally wide: split between the texts
		const bool midpoints = (int)(colCenter[0] - cw / 2 + 0.5f) < box.left + 3;
		int maxSpan = 1;
		for (int c = 0; c < nCols; c++)
			maxSpan = MAX(maxSpan, spanB[c] - spanA[c]);
		const float frame = (left0 - box.left) + (box.right - rightN);
		const float need = (frame + nCols * maxSpan) * sy;
		if (need > area.width())
			sy *= area.width() / need;
		const float cellW = (area.width() - frame * sy) / nCols;
		const int bottom = (int)(top + box.height() * sy);

		float dx = area.left;
		addSeg(box, box.left, left0, dx, dx + (left0 - box.left) * sy, top, bottom);
		dx += (left0 - box.left) * sy;

		Common::Array<float> cellX(nCols);
		for (int c = 0; c < nCols; c++) {
			const int lc = MIN<int>(c == 0 ? left0 : midpoints ? (spanB[c - 1] + spanA[c]) / 2 : (int)(colCenter[c] - cw / 2 + 0.5f), spanA[c] - 1);
			const int rc = MAX<int>(c == nCols - 1 ? rightN : midpoints ? (spanB[c] + spanA[c + 1]) / 2 : (int)(colCenter[c] + cw / 2 + 0.5f), spanB[c] + 1);
			cellX[c] = dx;
			const float gapDst = (cellW - (spanB[c] - spanA[c]) * sy) / 2;
			const int kl = MIN<int>(2, (spanA[c] - lc) / 2);
			const int kr = MIN<int>(2, (rc - spanB[c]) / 2);

			if (midpoints) {
				// Fill the button with a stretched column of its own background and
				// leave a thin black border around it
				const float textX = dx + gapDst;
				const float textW = (spanB[c] - spanA[c]) * sy;
				const int fl = fillX >= 0 ? fillX : spanA[c];
					addSeg(box, fl, fl + 1, dx + sy, textX, top, bottom);
				addSeg(box, spanA[c], spanB[c], textX, textX + textW, top, bottom);
				const int fr = fillX >= 0 ? fillX : spanB[c] - 1;
					addSeg(box, fr, fr + 1, textX + textW, dx + cellW - sy, top, bottom);
			} else {
				addSeg(box, lc, lc + kl, dx, dx + kl * sy, top, bottom);
				addSeg(box, lc + kl, spanA[c], dx + kl * sy, dx + gapDst, top, bottom);
				addSeg(box, spanA[c], spanB[c], dx + gapDst, dx + gapDst + (spanB[c] - spanA[c]) * sy, top, bottom);
				addSeg(box, spanB[c], rc - kr, dx + gapDst + (spanB[c] - spanA[c]) * sy, dx + cellW - kr * sy, top, bottom);
				addSeg(box, rc - kr, rc, dx + cellW - kr * sy, dx + cellW, top, bottom);
			}
			dx += cellW;
		}
		addSeg(box, rightN, box.right, dx, area.right, top, bottom);

		for (uint i = 0; i < texts.size(); i++) {
			const int c = indexOf(colReps, centersX[i], 6);
			const int r = indexOf(rowReps, centersY[i], 3);
			const float y0 = MAX<float>(rowCenter[r] - ph / 2, box.top);
			const float y1 = MIN<float>(rowCenter[r] + ph / 2, box.bottom);
			const Common::Rect hit((int)cellX[c], (int)(top + (y0 - box.top) * sy),
			                       (int)(cellX[c] + cellW), (int)(top + (y1 - box.top) * sy));
			addCell(texts[i], Common::Rect(), hit, false);
		}
		return true;
	}

	void addSeg(const Common::Rect &box, int sx0, int sx1, float dx0, float dx1, float top, int bottom) {
		if (sx1 <= sx0 || dx1 - dx0 < 1)
			return;
		addCell(Common::Rect(sx0, box.top, sx1, box.bottom),
		        Common::Rect((int)dx0, (int)top, (int)(dx1 + 0.5f), bottom), Common::Rect(), false);
	}

	// Shows the verb box (group 4, with its texts in group 6) above the inventory
	// box (group 5), both at the bottom of the area under the sentence line.
	void computeRegions(const Common::Rect &area) {
		Common::Rect sentence, boxes[2];
		bool hasSentence = false, hasBox[2] = { false, false };
		Common::Array<Common::Rect> texts, slots;
		for (uint i = 0; i < _elements.size(); i++) {
			const uint8 g = _elements[i].group;
			if (g == 7)
				slots.push_back(_elements[i].src); // Inventory slots, inside the group 5 box
			else if (g == 0)
				unite(sentence, hasSentence, _elements[i].src);
			else if (g == 4 || g == 5)
				unite(boxes[g - 4], hasBox[g - 4], _elements[i].src);
			else if (g == 6) {
				texts.push_back(_elements[i].src);
				unite(boxes[0], hasBox[0], _elements[i].src);
			}
		}

		const int areaW = area.width();
		const int areaH = area.height();
		const float sentenceScale = (float)areaW / _gameWidth;
		const float sentenceH = hasSentence ? MAX<int>(sentence.height(), kTextHeight) * sentenceScale : 0;
		const float gapSrc = 3;

		int maxW = 1;
		float srcH = 0;
		for (int b = 0; b < 2; b++) {
			if (!hasBox[b])
				continue;
			maxW = MAX(maxW, (int)boxes[b].width());
			srcH += boxes[b].height() + gapSrc;
		}

		float scale = (float)areaW / maxW;
		if (sentenceH + srcH * scale > areaH)
			scale = MAX<float>(areaH - sentenceH, 1) / srcH;

		float y = area.bottom;
		for (int b = 1; b >= 0; b--) {
			if (!hasBox[b])
				continue;
			const float h = boxes[b].height() * scale;
			y -= h;
			if (addStretchedBox(boxes[b], b == 0 ? texts : slots, area, scale, y, b == 0 ? -1 : boxes[b].left + 2)) {
				// Done
			} else {
				const int dw = (int)(boxes[b].width() * scale);
				const int dx = area.left + (areaW - dw) / 2;
				const Common::Rect draw(dx, (int)y, dx + MAX(dw, 1), (int)(y + h));
				addCell(boxes[b], draw, draw, true);
			}
			y -= gapSrc * scale;
		}

		if (hasSentence) {
			y -= sentenceH;
			addCell(Common::Rect(0, sentence.top, _gameWidth, sentence.top + MAX<int>(sentence.height(), kTextHeight)),
			        Common::Rect(area.left, (int)y, area.right, (int)(y + sentenceH)), Common::Rect(), false);
		}
	}

	Common::Array<Element> _elements;
	bool _regionMode = false;
	Block _blocks[2];
	bool _hasSentence = false;
	Common::Array<Cell> _cells;
	Common::Rect _area;
	int _gameWidth = 320;
};

#endif
