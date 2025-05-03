import sys
import csv
import time


'''
Assumes that there is an source.csv of format

`
lat,lon
37.7749,-122.4194
34.0522,-118.2437
...
`

to generate a kml file with user input, stores in output_kml directory

Runs with arguments:
python generate_kml.py <source_csv> optional:<output_kml>

output_kml is default the date+time of file generation

'''

def generate_kml(source_csv, output_kml=str(time.strftime("output-%Y-%m-%d-%H-%M-%S"))):
    
    try:
        with open(source_csv, 'r') as csv_file:
            reader = csv.reader(csv_file)
            header = next(reader)

            output_kml = "./output_kml/" + output_kml
            
            with open(output_kml, 'w') as kml_file:
                kml_file.write('<?xml version="1.0" encoding="UTF-8"?>\n')
                kml_file.write('<kml xmlns="http://www.opengis.net/kml/2.2">\n')
                kml_file.write(' <Document>\n')
                
                hotspot_count = 1
                for i, row in enumerate(reader):
                    lat, lon = row
                    print("=========================")
                    print(f"LABELLING AT {lat}, {lon}")

                    placemark_name = ""
                    placemark_input = input(f"What is the name of the placemark? (default is Hotspot {hotspot_count}): ")
                    if (len(placemark_input) == 0):
                        placemark_name = f"Hotspot {hotspot_count}"
                        hotspot_count += 1
                    else:
                        placemark_name = placemark_input
                    print(f"Name set to: {placemark_name}")
                    
                    description = ""
                    description = input(f"What is the description? (default is blank): ")
                    print(f"Description set to: '{description}'")

                    kml_file.write('  <Placemark>\n')
                    kml_file.write(f'   <name>{placemark_name}</name>\n')
                    if description != "":
                        kml_file.write(f'   <description>{description}</description>\n')
                    kml_file.write('   <Point>\n')
                    kml_file.write(f'    <coordinates>{lon},{lat},0</coordinates>\n')
                    kml_file.write('   </Point>\n')
                    kml_file.write('  </Placemark>\n')
                
                kml_file.write(' </Document>\n')
                kml_file.write('</kml>\n')
        print(f"KML file generated successfully: {output_kml}")
    except Exception as e:
        print(f"ERROR: {e}")

if __name__ == '__main__':
    args = sys.argv[1:]
    if len(args) != 1:
        print("ERROR: invalid number of arguments")
        print("Usage: python generate_kml.py <source_csv> optional:<output_kml>")
    else:
        generate_kml(*args)