This Visual Studio 2010 solution is used to create vl.lib and vl.dll, used by the Matlab vl_sift function.
To compile the function, use vl_compile located under the toolbox directory.
It is necessary to compile the library and the Matlab function in order to obtain in output the peak of the DoG, used to compute the relevance of the features in CDVS. The standard version of vl_sift doesn't provide in output this value.
