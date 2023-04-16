﻿/*****************************************************************************
 * 
 * ReoGrid - .NET Spreadsheet Control
 * 
 * http://reogrid.net/
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * Author: Jing <lujing at unvell.com>
 *
 * Copyright (c) 2012-2016 Jing <lujing at unvell.com>
 * Copyright (c) 2012-2016 unvell.com, all rights reserved.
 * 
 ****************************************************************************/

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using RGFloat = System.Single;

namespace unvell.ReoGrid.Graphics
{
	/// <summary>
	/// Represents point information that includes the x-coordinate value and y-coordinate value.
	/// </summary>
	[Serializable]
	public struct Point
	{
		/// <summary>
		/// Get or set the value on x-coordinate.
		/// </summary>
		public RGFloat X { get; set; }

		/// <summary>
		/// Get or set the value on y-coordinate.
		/// </summary>
		public RGFloat Y { get; set; }

		/// <summary>
		/// Create point by specified x-coordinate value and y-coordinate value.
		/// </summary>
		/// <param name="x">Value on x-coordinate.</param>
		/// <param name="y">Value on y-coordinate.</param>
		public Point(RGFloat x, RGFloat y)
			: this()
		{
			this.X = x;
			this.Y = y;
		}

		/// <summary>
		/// Compare two points to check whether or not they are same.
		/// </summary>
		/// <param name="obj">Another object to be compared with this point.</param>
		/// <returns>True if two points are same; Otherwise return false.</returns>
		public override bool Equals(object obj)
		{
			if (!(obj is Point)) return false;

			var size2 = (Point)obj;

			return this.X == size2.X && this.Y == size2.Y;
		}

		/// <summary>
		/// Get hash code of this point.
		/// </summary>
		/// <returns></returns>
		public override int GetHashCode()
		{
			return base.GetHashCode();
		}

		/// <summary>
		/// Convert point into string. (Format: {x, y})
		/// </summary>
		/// <returns>String converted from this point.</returns>
		public override string ToString()
		{
			return string.Format("{{{0,2}, {1,2}}}", this.X, this.Y);
		}

		/// <summary>
		/// Compare two points to check whether or not they are same.
		/// </summary>
		/// <param name="size1">First point to be compared.</param>
		/// <param name="size2">Second point to be compared.</param>
		/// <returns>True if two points are same; Otherwise return false.</returns>
		public static bool operator ==(Point size1, Point size2)
		{
			return size1.X == size2.X && size1.Y == size2.Y;
		}

		/// <summary>
		/// Compare two points to check whether or not they are not same.
		/// </summary>
		/// <param name="size1">First point to be compared.</param>
		/// <param name="size2">Second point to be compared.</param>
		/// <returns>True if two points are not same; Otherwise return false.</returns>
		public static bool operator !=(Point size1, Point size2)
		{
			return size1.X != size2.X || size1.Y != size2.Y;
		}

		/// <summary>
		/// Transform point by specified matrix.
		/// </summary>
		/// <param name="p">Point to be transformed.</param>
		/// <param name="m">Matrix used to calculate the result of transform.</param>
		/// <returns>A transformed point from specified matrix.</returns>
		public static Point operator *(Point p, Matrix3x2f m)
		{
			return new Point(p.X * m.a1 + p.Y * m.a2 + m.a3, p.X * m.a1 + p.Y * m.b2 + m.b3);
		}

		#region Platform Associated
		/// <summary>
		/// Convert System.Drawing.Point to unvell.ReoGrid.Graphics.Point.
		/// </summary>
		/// <param name="p">Point of unvell.ReoGrid.Graphics.Point.</param>
		/// <returns>Point of System.Drawing.Point.</returns>
		public static implicit operator Point(System.Drawing.Point p)
		{
			return new Point(p.X, p.Y);
		}
		/// <summary>
		/// Convert System.Drawing.PointF to unvell.ReoGrid.Graphics.Point.
		/// </summary>
		/// <param name="p">Point of unvell.ReoGrid.Graphics.Point.</param>
		/// <returns>Point of System.Drawing.PointF.</returns>
		public static implicit operator Point(System.Drawing.PointF p)
		{
			return new Point(p.X, p.Y);
		}
		/// <summary>
		/// Convert unvell.ReoGrid.Graphics.Point to System.Drawing.Point.
		/// </summary>
		/// <param name="p">Point of unvell.ReoGrid.Graphics.Point.</param>
		/// <returns>Point of System.Drawing.Point.</returns>
		public static explicit operator System.Drawing.Point(Point p)
		{
			return new System.Drawing.Point((int)Math.Round(p.X), (int)Math.Round(p.Y));
		}
		/// <summary>
		/// Convert unvell.ReoGrid.Graphics.Point to System.Drawing.Point.
		/// </summary>
		/// <param name="p">Point of unvell.ReoGrid.Graphics.Point.</param>
		/// <returns>Point of System.Drawing.PointF.</returns>
		public static implicit operator System.Drawing.PointF(Point p)
		{
			return new System.Drawing.PointF(p.X, p.Y);
		}

		#endregion // Platform Associated
	}

}
