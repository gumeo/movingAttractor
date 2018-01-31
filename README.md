# movingAttractor

Read more about this in my blog post [here](https://gumeo.github.io/post/visualizing-strange-attractors/). I built this on Ubuntu 16.04, you will need to have openGL to compile this.

For using this type `make` and then `./tool` in the terminal. This version only produces the results in a viewing window, if you want to save the images to a file, uncomment the line number 223, which is using `screenshot_ppm` to save the images. Be careful then to also define what the files should be called and where they should be saved.

Here is an example of a randered attractor

<center>
<video width="640" controls="controls">
<source src="./video.mp4" type="video/mp4">
</video>
</center>
