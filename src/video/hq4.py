# $Id$

from hqcommon import (
	computeNeighbours, makeLite as commonMakeLite,
	permuteCases, printSubExpr, printText, writeBinaryFile
	)

from collections import defaultdict
from copy import deepcopy

class Parser(object):

	def __init__(self):
		self.fileName = 'HQ4xScaler.in'
		self.pixelExpr = [ [ None ] * 16 for _ in range(1 << 12) ]
		self.__parse()
		sanityCheck(self.pixelExpr)

	@staticmethod
	def __filterSwitch(stream):
		log = False
		inIf = False
		for line in stream:
			line = line.strip()
			if line == 'switch (pattern) {':
				log = True
			elif line == '}':
				if inIf:
					inIf = False
				elif log:
					break
			elif line.startswith('//pixel'):
				line = line[2 : ]
			elif line.startswith('if'):
				inIf = True
			if log:
				yield line

	def __parse(self):
		cases = []
		subCases = range(1 << 4)
		for line in self.__filterSwitch(file(self.fileName)):
			if line.startswith('case'):
				cases.append(int(line[5 : line.index(':', 5)]))
			elif line.startswith('pixel'):
				subPixel = int(line[5], 16)
				assert 0 <= subPixel < 16
				expr = line[line.index('=') + 1 : ].strip()
				self.__addCases(cases, subCases, subPixel, expr[ : -1])
			elif line.startswith('if'):
				index = line.find('edge')
				assert index != -1
				index1 = line.index('(', index)
				index2 = line.index(',', index1)
				index3 = line.index(')', index2)
				pix1s = line[index1 + 1 : index2].strip()
				pix2s = line[index2 + 1 : index3].strip()
				assert pix1s[0] == 'c', pix1s
				assert pix2s[0] == 'c', pix2s
				pix1 = int(pix1s[1:])
				pix2 = int(pix2s[1:])
				if pix1 == 2 and pix2 == 6:
					subCase = 0
				elif pix1 == 6 and pix2 == 8:
					subCase = 1
				elif pix1 == 8 and pix2 == 4:
					subCase = 2
				elif pix1 == 4 and pix2 == 2:
					subCase = 3
				else:
					assert False, (line, pix1, pix2)
				subCases = [ x for x in range(1 << 4) if x & (1 << subCase) ]
			elif line.startswith('} else'):
				subCases = [ x for x in range(1 << 4) if x not in subCases ]
			elif line.startswith('}'):
				subCases = range(1 << 4)
			elif line.startswith('break'):
				cases = []
				subCases = range(1 << 4)

	def __addCases(self, cases, subCases, subPixel, expr):
		pixelExpr = self.pixelExpr
		for case in cases:
			for subCase in subCases:
				weights = [ 0 ] * 16
				if expr.startswith('interpolate'):
					factorsStr = expr[
						expr.index('<') + 1 : expr.index('>')
						].split(',')
					pixelsStr = expr[
						expr.index('(') + 1 : expr.index(')')
						].split(',')
					assert len(factorsStr) == len(pixelsStr)
					for factorStr, pixelStr in zip(factorsStr, pixelsStr):
						factor = int(factorStr)
						pixelStr = pixelStr.strip()
						assert pixelStr[0] == 'c'
						pixel = int(pixelStr[1 : ]) - 1
						weights[pixel] = factor
				else:
					assert expr[0] == 'c'
					weights[int(expr[1 : ]) - 1] = 1
				pixelExpr[(case << 4) | subCase][subPixel] = weights

def sanityCheck(pixelExpr):
	'''Check various observed properties.
	'''
	subsets = [ (5, 4, 2, 1), (5, 6, 2, 3), (5, 4, 8, 7), (5, 6, 8, 9) ]

	for case, expr in enumerate(pixelExpr):
		for corner in expr:
			# Weight of the center pixel is never zero.
			# TODO: This is not the case for 4x, is that expected or a problem?
			#assert corner[4] != 0
			# Sum of weight factors is always a power of two.
			total = sum(corner)
			assert total in (1, 2, 4, 8, 16), total
			# There are at most 3 non-zero weights, and if there are 3 one of
			# those must be for the center pixel.
			numNonZero = sum(weight != 0 for weight in corner)
			assert numNonZero <= 3, (case, corner)
			assert numNonZero < 3 or corner[4] != 0

		# Subpixel depends only on the center and three neighbours in the
		# direction of the subpixel itself.
		# TODO: This is not the case for 4x, is that expected or a problem?
		#for corner, subset in zip(expr, subsets):
			#for pixel in range(9):
				#if (pixel + 1) not in subset:
					#assert corner[pixel] == 0, corner

def genSwitch(pixelExpr):
	exprToCases = defaultdict(list)
	for case, expr in enumerate(pixelExpr):
		exprToCases[tuple(tuple(subExpr) for subExpr in expr)].append(case)
	#print exprToCases
	yield 'switch (pattern) {\n'
	for cases, expr in sorted(
		( sorted(cases), expr )
		for expr, cases in exprToCases.iteritems()
		):
		for case in cases:
			yield 'case %d:\n' % case
		for subPixel, subExpr in enumerate(expr):
			yield '\tpixel%s = %s;\n' % (
				hex(subPixel)[2 : ], printSubExpr(subExpr)
				)
		yield '\tbreak;\n'
	yield 'default:\n'
	yield '\tassert(false);\n'
	yield '\tpixel0 = pixel1 = pixel2 = pixel3 =\n'
	yield '\tpixel4 = pixel5 = pixel6 = pixel7 =\n'
	yield '\tpixel8 = pixel9 = pixela = pixelb =\n'
	yield '\tpixelc = pixeld = pixele = pixelf = 0; // avoid warning\n'
	yield '}\n'

def formatLiteWeightsTable(pixelExpr):
	for case, expr in enumerate(pixelExpr):
		yield '// %d\n' % case
		for weights in expr:
			factor = 256 / sum(weights)
			for c in (3, 4, 5):
				yield ' %3d,' % min(255, factor * weights[c])
			yield '\n'

def formatOffsetsTable(pixelExpr):
	xy = computeXY(pixelExpr)
	for case, expr in enumerate(pixelExpr):
		yield '// %d\n' % case
		for subPixel, weights in enumerate(expr):
			for i in range(2):
				t = xy[case][subPixel][i]
				x = min(255, (1 if t is None else (t % 3)) * 128)
				y = min(255, (1 if t is None else (t / 3)) * 128)
				yield ' %3d, %3d,' % (x, y)
			yield '\n'

def formatWeightsTable(pixelExpr):
	xy = computeXY(pixelExpr)
	for case, expr in enumerate(pixelExpr):
		yield '// %d\n' % case
		for subPixel, weights in enumerate(expr):
			factor = 256 / sum(weights)
			for c in (xy[case][subPixel][0], xy[case][subPixel][1], 4):
				yield ' %3d,' % min(255, 0 if c is None else factor * weights[c])
			yield '\n'

def computeXY(pixelExpr):
	return [
		[ computeNeighbours(weights) for weights in expr ]
		for expr in pixelExpr
		]

def genHQOffsetsTable(pixelExpr):
	xy = computeXY(pixelExpr)
	for case, expr in enumerate(pixelExpr):
		for subPixel, weights in enumerate(expr):
			for i in range(2):
				t = xy[case][subPixel][i]
				x = min(255, (1 if t is None else (t % 3)) * 128)
				y = min(255, (1 if t is None else (t / 3)) * 128)
				yield x
				yield y

def genHQWeightsTable(pixelExpr):
	xy = computeXY(pixelExpr)
	for case, expr in enumerate(pixelExpr):
		for subPixel, weights in enumerate(expr):
			factor = 256 / sum(weights)
			for c in (xy[case][subPixel][0], xy[case][subPixel][1], 4):
				yield min(255, 0 if c is None else factor * weights[c])

def genHQLiteOffsetsTable(pixelExpr):
	offset_x = ( 48,  16, -16, -48,  48,  16, -16, -48,  48,  16, -16, -48,  48,  16, -16, -48)
	offset_y = ( 48,  48,  48,  48,  16,  16,  16,  16, -16, -16, -16, -16, -48, -48, -48, -48)
	for case, expr in enumerate(pixelExpr):
		for subPixel, weights in enumerate(expr):
			for c in (0, 1, 2, 6, 7, 8):
				assert weights[c] == 0
			assert weights[3] == 0 or weights[5] == 0
			factor = sum(weights)
			x = offset_x[subPixel] + 128
			y = offset_y[subPixel] + 128
			if weights[5] == 0:
				x -= 128 * weights[3] / factor
			else:
				x += 128 * weights[5] / factor
			#print x, y
			assert 0 <= x
			assert x <= 255
			yield x
			yield y

def makeLite(pixelExpr, preferC6subPixels):
	# TODO: Rewrite hqcommon.makeLite() so it doesn't change its input.
	liteExpr = deepcopy(pixelExpr)
	commonMakeLite(liteExpr, preferC6subPixels)
	return liteExpr

class Variant(object):

	def __init__(self, pixelExpr, lite, narrow, table):
		self.lite = lite
		self.narrow = narrow
		self.table = table
		if lite:
			pixelExpr = makeLite(pixelExpr, (2, 3, 6, 7, 10, 11, 14, 15) if table else ())
		if narrow:
			assert False
		pixelExpr = permuteCases(
			(5, 0, 4, 6, 3, 10, 11, 2, 1, 9, 8, 7)
			if table else
			(2, 9, 7, 4, 3, 10, 11, 1, 8, 0, 6, 5),
			pixelExpr
			)
		self.pixelExpr = pixelExpr

if __name__ == '__main__':
	parser = Parser()

	fullTableVariant = Variant(parser.pixelExpr, lite = False, narrow = False, table = True )
	liteTableVariant = Variant(parser.pixelExpr, lite = True,  narrow = False, table = True )

	#printText(formatOffsetsTable(fullTableVariant.pixelExpr))
	#printText(formatWeightsTable(fullTableVariant.pixelExpr))
	#printText(formatLiteWeightsTable(liteTableVariant.pixelExpr))

	writeBinaryFile(
		'HQ4xOffsets.dat',
		genHQOffsetsTable(fullTableVariant.pixelExpr)
		)
	writeBinaryFile(
		'HQ4xWeights.dat',
		genHQWeightsTable(fullTableVariant.pixelExpr)
		)
	writeBinaryFile(
		'HQ4xLiteOffsets.dat',
		genHQLiteOffsetsTable(liteTableVariant.pixelExpr)
		)
	# Note: HQ4xLiteWeights.dat is not needed, since interpolated texture
	#       offsets can perform all the blending we need.

	#printText(genSwitch(Variant(
		#parser.pixelExpr, lite = False, narrow = False, table = False
		#).pixelExpr))