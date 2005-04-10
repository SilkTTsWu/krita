/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <qrect.h>

#include <kdebug.h>

#include "kis_vec.h"
#include "kis_brush.h"
#include "kis_global.h"
#include "kis_paint_device.h"
#include "kis_painter.h"
#include "kis_types.h"
#include "kis_paintop.h"

#include "kis_airbrushop.h"

KisPaintOp * KisAirbrushOpFactory::createOp(KisPainter * painter)
{ 
	KisPaintOp * op = new KisAirbrushOp(painter); 
	return op; 
}


KisAirbrushOp::KisAirbrushOp(KisPainter * painter)
	: super(painter) 
{
}

KisAirbrushOp::~KisAirbrushOp() 
{
}

void KisAirbrushOp::paintAt(const KisPoint &pos,
			    const double pressure,
			    const double /*xTilt*/,
			    const double /*yTilt*/)
{
// See: http://www.sysf.physto.se/~klere/airbrush/ for information
// about _real_ airbrushes.
//
// Most graphics apps -- especially the simple ones like Kolourpaint
// and the previous version of this routine in Krita took a brush
// shape -- often a simple ellipse -- and filled that shape with a
// random 'spray' of single pixels.
//
// Other, more advanced graphics apps, like the Gimp or Photoshop,
// take the brush shape and paint just as with the brush paint op,
// only making the initial dab more transparent, and perhaps adding
// extra transparence near the edges. Then, using a timer, when the
// cursor stays in place, dab upon dab is positioned in the same
// place, which makes the result less and less transparent.
//
// What I want to do here is create an airbrush that approaches a real
// one. It won't use brush shapes, instead going for the old-fashioned
// circle. Depending upon pressure, both the size of the dab and the
// rate of paint deposition is determined. The edges of the dab are
// more transparent than the center, with perhaps even some fully
// transparent pixels between the near-transparent pixels.
//
// By pressing some to-be-determined key at the same time as pressing
// mouse-down, one edge of the dab is made straight, to simulate
// working with a shield.
//
// Tilt may be used to make the gradients more realistic, but I don't
// have a tablet that supports tilt.
//
// Anyway, it's exactly twenty years ago that I have held a real
// airbrush, for the first and up to now the last time...
//

	if (!m_painter) return;

	KisPaintDeviceSP device = m_painter -> device();

	// For now: use the current brush shape -- it beats calculating
	// ellipes and cones, and it shows the working of the timer.
	if (!device) return;

	KisBrush * brush = m_painter -> brush();
	double oldPressure = m_painter -> pressure();
	KisPaintDeviceSP dab = m_painter -> dab();

	KisPoint hotSpot = brush -> hotSpot(pressure);
	KisPoint pt = pos - hotSpot;

	Q_INT32 x;
	double xFraction;
	Q_INT32 y;
	double yFraction;

	splitCoordinate(pt.x(), &x, &xFraction);
	splitCoordinate(pt.y(), &y, &yFraction);

	if (brush -> brushType() == IMAGE || brush -> brushType() == PIPE_IMAGE) {
		dab = brush -> image(device -> colorStrategy(), pressure, xFraction, yFraction);
	}
	else {
		KisAlphaMaskSP mask = brush -> mask(pressure, xFraction, yFraction);
		dab = computeDab(mask);
	}

	m_painter -> setDab(dab); // Cache dab for future paints in the painter.
	m_painter -> setPressure(pressure); // Cache pressure in the current painter.

	QRect dabRect = QRect(0, 0, brush -> maskWidth(pressure), brush -> maskHeight(pressure));
	
	KisImage * image = device -> image();
	
	if (image != 0) {
		QRect imageRect = image -> bounds();
		if (x > imageRect.width()
			|| y > imageRect.height()
			|| x + dabRect.width() < 0
			|| y < + dabRect.height() < 0) return;
	}
	
	if (dabRect.isNull() || dabRect.isEmpty() || !dabRect.isValid()) return;
	
	m_painter -> bltSelection( x,  y,  m_painter -> compositeOp(), dab.data(), OPACITY_OPAQUE / 50, 0, 0, dabRect.width(), dabRect.height());
	m_painter -> addDirtyRect(QRect(x, y, dabRect.width(), dabRect.height()));

}
