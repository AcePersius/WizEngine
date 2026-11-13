# WizEngine, the graphics/simulation/game-development engine

## The Why Behind This Project?
    TLDR: This is my attempt at building a graphics/simulation/game-development engine.

    How I Got To This Point:
    This project is the much stronger, cooler, older brother to my Software Rasterizer project with followed my Chip-8 emulator. I decided to take the Harvard CS50X course online
    while deployed. For that course I built some foundations in programming and arrived at the end where we had to build some kind of final project. For better or
    worse I really liked the programming in C weeks which lead me to wanting to use that for a project. Unfortunately it kind of felt like the coursework set us up more for web
    development than lower C coding, but I left with a large ego and naitivity than lead me to eventually settling on doing a chip-8 emulator. For those who don't know, a chip-8 
    emulator emulates a CHIP-8 virtual machine/interpreter used to run CHIP-8 programs such as old game roms from the 1970's - 2000's era. Without boring with too much more detail on that,
    using some excellent guides by some people smarter than I, I managed to finish the project and really felt like I had written something of merit. That lead me into exploring more into
    c related projects and trying to find out what I wanted my niche to be. Eventually I landed on Graphics Programming as I loved games, and even more than games I love knowing
    how a program works. As a beginner during the course as well as now as I learn more and more day to day, I get extremely frustrated with not knowing how software works. OThf all
    things in the world programming should be the most fundamentally straight forward things in the world. So instead of just using some software made by brilliant minds and worked
    on by hundreds or thousands of people like Unity, Unity, Godot, or Gamemaker, I decided to build my own Game engine from the ground up so that I can avoid bloat and know how 
    ever detail works. This lead me to attempting to learn OpenGL with all of the resources and amazing free online PDF, I was so darn frustrated because I One: was a beginner who
    didn't know what a Vertex Buffer Object or what a Graphics Pipeline really was and Two: didn't understand the abstractions OpenGl wanted me to use. So in order to learn graphics
    programming and do something that I believed deserved more merit than my Chip8 project, I did the sensible thing (LOL) and decided to build a software rasterizer after watching a
    Tsoding video on youtube. Without going into too much detail, here I'll just say I learned a LOT from that project (It was also here that I switched over to C++ for various
    reasons, primarily relating to its frequent use in GPU and game dec work as well as its similarity to C). It is an extremely inefficient, nonperformant program that could
    probably be described as a bit of a mess, but it took me from nothing to rendering a model on screen + moving the camera to inspect it. The project forced me to learn from the 
    ground up which helped me a lot, but at the end of the day it was a learning resource more akin to proving I can rather than actually being "My Project". However, with that 
    new found knowledge it lead me to my project today "WizEngine". 
    
    WizEngine (More Lore):
    
    I had reached what I felt at the time was lot of fundamentals for graphics programming, but I knew learning an API would be diffucult. Here I decided to give Vulkan a try as it is 
    advertised as a "do it all yourself from the bottom up" api which should be following a theme you might be picking up on. Now let me introduce the amazing resource that really
    made all of this possible: https://docs.vulkan.org/tutorial/latest/00_Introduction.html . Vulkan.org has made so a great tutorial on the exact topic I wanted to learn about 
    and explore. Armed with a lot of the skills I needed, ambition, and a great resource I began my work on 2026/09/26. That leads me to today where I am making my first real submission
    to github with my software which I hope to bloom into something useful if not for others atleast my own goals and ambitions.

## Currently Implemented:
    Graphics pipeline including:
        Vertex buffers
        Uniform buffers
        Texture mapping
        Depth buffering
        Generating Mipmaps
        Multisampling
    Loading models AND Migrating to Modern Asset Formats: glTF and KTX2
    Rendering Multiple Objects
    Complete Compute Shader Pipeline Submitted on a GraphicsCompute Queue
    Multithreading

## WizEngine Docs:
    Please take time to look at the WizEngine Vulkan Renderer Implementation Reference.
    1. I spent a lot of time (about a week straight) writing up Documentation then fixing it with AI to make it less gargled mess of steps and more coherent.
    I've read through and corrected all the mistakes and verified nothing changed in heart other than wording from my original writing. I will warn that those docs aren't
    completely up to date with the current iteration of the engine, but they cover everything before Compute Shader, Migrating to Modern Asset Formats: gltf and KTX2, and
    multithreading. 
    2. If a person were to be also following the Vulkan tutorial like me then I could see it being extremely useful. Eventually in the future that is hopefully
    nearer than far, I plan on reaching another stopping point in the engine to go back and update the docs, but the time isn't quite there yet. I still think it holds
    a lot of value in understand the Vulkan Basics of how 0 to rendering models on screen work though, and make everything that come after than seem much easier.

## Why Am I Uploading My Unfinished Software?
    As a humble man who decided one day to start learn programming while deployed, I don't know proper github etiquette yet, but I feel as though I have accomplished enough 
    in my project to warrant this upload. I have done everything in the Vulkan.org tutorial up to the "Building a Simple Engine" section (with exceptions to Ecosystem stuff, Profiles, and Raytracing).

# AI Use Disclaimer
    I have used AI to assist in my software development so far. I try to use it as a guide and debugger to ask questions and bridge tutorial gaps, but I don't like using it to as an
    "AI make X thing for me" type of resource. My primary goal in this project is exposure and learning with a side effect of building a program, not the other way around. That was MY testimony.

## License

WizEngine is licensed under the MIT License. See [LICENSE](LICENSE) for details.

Third-party libraries and assets retain their respective licenses. See
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for details.