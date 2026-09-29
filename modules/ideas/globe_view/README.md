# Globe view

A spinnable globe when zoomed all the way out, instead of the flat world map, as Apple Maps and Guru Maps show it.
Land cover (../../landcover) and relief (../../relief) would wrap onto it. Needs a projection in the renderer
(drape_frontend) that upstream doesn't have, so it's a large module; the flat map stays from about zoom 4 in.
