# Open Space Data Link Protocol

This is the open-source, OS-independent library that implements the following standards:

[1] [CCSDS Space Packet](https://public.ccsds.org/Pubs/133x0b1c2.pdf)

[2] [CCSDS TM Space Data Link Protocol](https://public.ccsds.org/Pubs/132x0b2.pdf)

[3] [CCSDS TC Space Data Link Protocol](https://public.ccsds.org/Pubs/232x0b3.pdf)

### Coding style
For the C code, `osdlp` uses a slightly modified version of the 
**Stroustrup** style, which is a nicer adaptation of the well known K&R style.

At the root directory of the project there is the `astyle` options 
file `.astylerc` containing the proper configuration.
Developers can import this configuration to their favorite editor. 
In addition the `hooks/pre-commit` file contains a Git hook, 
that can be used to perform before every commit, code style formatting
with `astyle` and the `.astylerc` parameters.
To enable this hook developers should copy the hook at their `.git/hooks` 
directory. 
Failing to comply with the coding style described by the `.astylerc` 
will result to failure of the automated tests running on our CI services. 
So make sure that you either import on your editor the coding style rules 
or use the `pre-commit` Git hook.


## Website and Contact
For more information about the project and Libre Space Foundation please visit our [site](https://libre.space/)
and our [community forums](https://community.libre.space).

## License

![Libre Space Foundation](docs/assets/LSF_HD_Horizontal_Color1-300x66.png) 
&copy; 2016-2019 [Libre Space Foundation](https://libre.space).

Licensed under the [GPLv3](LICENSE).
