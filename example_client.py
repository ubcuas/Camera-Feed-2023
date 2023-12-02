import cv2
import numpy as np
import requests
import base64
import click

@click.group()
@click.option('--url', '-u', required=True, help='Camera server URL')
@click.pass_context
def cli(ctx, url):
    click.echo(f'Using URL: {url}')
    ctx.obj = {'url': url}


@cli.command('image')
@click.option('--number', '-n', default=1, type=int)
@click.pass_context
def image(ctx, number):
    """Take specified number of images."""
    url = ctx.obj['url']
    click.echo(f'Taking {number} images from {url}...')
    # for i in range(num_images):
    #     response = requests.get(f'{url}/image')
    #     data = response.json()
    #     image_data = data.get('image')
    #     image = base64.b64decode(image_data)
    #     timestamp = data.get('timestamp')
    #     print(f'Saved to ./images/{timestamp}.jpg')
    #     with open(f'./images/{timestamp}.jpg', 'wb') as out:
    #         out.write(image)


@cli.command()
@click.option('--config', '-c', type=(str, str), multiple=True, required=True)
@click.pass_context
def setup(ctx, config):
    """Setup using the provided configuration."""
    url = ctx.obj['url']
    click.echo(f'Setting up with the following configuration and URL {url}:')
    click.echo(config)


if __name__ == '__main__':
    cli()

# def main():
#     for i in range(10):
#         response = requests.get(f'{url}/image')
#         data = response.json()
#         image_data = data.get('image')
#         image = base64.b64decode(image_data)
#         timestamp = data.get('timestamp')
#         print(timestamp)
#         with open(f'./images/{timestamp}.jpg', 'wb') as out:
#             out.write(image)

# def main():
#     response = requests.get(f'{url}/test')
#     data = response.json()
#     image_data = data.get('image')
#     image = base64.b64decode(image_data)
#     timestamp = data.get('timestamp')
#     print(timestamp)
#     with open('./images/b64test.jpg', 'wb') as out:
#         out.write(image)
#
#
# if __name__ == '__main__':
#     main()
