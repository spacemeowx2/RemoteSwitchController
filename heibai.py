from PIL import Image

out = Image.new('1', (320, 120), 1)
pixels = out.load()
with Image.open('mm.bmp') as img:
	for x in range(320):
		for y in range(120):
			t = img.getpixel((x, y))
			avg = 1.0 * sum(t) / len(t)
			pixels[x, y] = 1 if avg > 170 else 0

out.save('mm.png')
