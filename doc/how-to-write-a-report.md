  - [Report Guidelines for INF-2201](#report-guidelines-for-inf-2201)
  - [Suggested Scientific Paper
    Structure](#suggested-scientific-paper-structure)
  - [Citations and Sources](#citations-and-sources)
      - [Source Quality](#source-quality)
  - [Advice on Writing in General](#advice-on-writing-in-general)
  - [Typesetting with LaTeX](#typesetting-with-latex)

As you learn to write code, it is also important to learn how to write
*about* your code. The reports you write alongside your assignments are
a good chance to practice this kind of writing.

# Report Guidelines for INF-2201

  - **Structure: scientific paper**
    
    Your report should follow the basic structure of a scientific paper:
    introduce the system you built, review background concepts, describe
    the system, describe how it was evalutated, and describe the results
    of that evaluation. See below for a more detailed structure
    suggestion.

  - **Length: around 4 pages**
    
    Four pages is a typical length for a short paper at a computer
    science conference, and a short conference paper is the ideal format
    for a report in this course. Conference papers typically use a dense
    two-column layout (see the LaTeX template in the project
    repository), so if you use a roomier layout then your paper will
    probably have more pages. Either way, report length will not be
    strictly enforced. It is okay to go over or under, but if your
    report strays too far from the recommended length it is probably a
    bad sign.

  - **Citations: required**
    
    You must cite your sources. You will not necessarily have to do a
    lot of research in this class, but when you talk about operating
    systems concepts in your report, we expect you to cite the source of
    your information, even if it is just the textbook.

  - **File format: PDF**
    
    Your finished report should be in PDF format. If you are using a
    typesetting language like LaTeX, render to PDF before hand-in. If
    you are using a word processor like Word, export to PDF before
    hand-in.

  - **AI tools: discouraged but not banned. Use must be declared**
    
    In this course, we do not forbid you from using generative A.I.
    tools like ChatGPT, but we do discourage their use. We encourage you
    to write for yourself because it is a good exercise in collecting
    your thoughts and expressing them clearly. Good writing takes
    practice, and these reports are a chance to practice.
    
    If you do use ChatGPT or other generative AI tools, you must add a
    section to your report explaining what tools you used and how you
    used them. For example, if you used a tool to generate a draft that
    you then edited, say that.

# Suggested Scientific Paper Structure

You should structure your reports like a scientific paper. The correct
structure for a scientific paper depends on the field, subfield, and
even the specific project. But for systems research in computer science,
the following structure tends to work well. We recommend starting with a
structure like this and adapting it to suit your needs.

1.  **Introduction**
    
    A brief overview of your system and the fundamental problem it is
    trying to solve.

2.  **Background**
    
    A brief review of concepts necessary to understand your solution. In
    the case of Project 2 in this course, this would include concepts
    like processes, threads, context switching, scheduling, user space
    vs kernel space, and system calls.

3.  **Related Work**
    
    A compact review of other work in your project’s specific subfield.
    This is where you cite most of the things that you read while
    researching your project. What other projects inspired yours? What
    are you building on? What are you competing with?
    
    In this course you will not be doing a great deal of research, but
    if you, say, read up on how Linux does scheduling before you
    implemented your own scheduler, this the place to write about that.

4.  **System Description**
    
    Actually describe the system you built. We like to describe a system
    at four different layers of abstraction:
    
    1.  **Idea:** Start with an extremely high-level “elevator pitch”
        for your system. What is the main idea that motivates it and
        sets it apart from similar existing systems?
        
        Since the assignments have relatively fixed requirements, you
        will probably not have a lot to write here. You can omit this
        subsection or you can restate the requirements, the problem you
        are trying to solve.
    
    2.  **Architecture:** Then begin describing the system itself, still
        at a very high level. What are its major components and how do
        they connect? What are the most essential factors that shaped
        the highest-level structure of your system?
    
    3.  **Design:** Now go a little deeper, but not too detailed. What
        are the smaller components inside the major ones? What factors
        shaped your design decisions as you fleshed out the
        architecture? Give pseudocode for important algorithms or
        protocols that make your system unique. At this level, someone
        reading your report should be able to write a similar system in
        a different programming language or for a different
        architecture.
    
    4.  **Implementation:** Finally, describe implementation details
        such as programming language used and target hardware.

5.  **Evaluation**
    
    Describe the methods used to evaluate the system. Define the metrics
    used to measure performance, and the procedures used to gather
    metrics. Include details like the specs of the hardware that ran the
    experiments. Do not include results yet. Those go in the next
    section.

6.  **Results**
    
    Give the results of the evaluation, including data tables and plots.
    Describe your results in a very concrete, quantitative way. Just the
    facts. Qualitative discussion can go in the next section.

7.  **Discussion**
    
    Discuss your solution and your experiment results in a more
    qualitative way. What are the positives and negatives? Discuss
    unexpected results and their implications.

8.  **Future Work**
    
    Discuss possible ways to improve or expand the system. Discuss known
    bugs and how to fix them.

9.  **Conclusion**
    
    Briefly restate the key points of your work and wrap up the paper.

Since the parameters of your assignments in this course are relatively
narrow, you may not have a lot of content for every section listed. In
that case you may shorten or combine sections, but try to follow the
spirit of the template.

# Citations and Sources

Of course, you must cite the sources of information that you include in
your report. We encourage you to do a little more research into the
concepts and systems that we are studying and implementing, and to
include that research in your report.

When you first search for information, you will probably first find
overviews like Wikipedia and tutorial sites. It is fine to use these to
get started in learning about a topic, but you should try to dig beyond
such sites to more authoritative sources. Where did the tutorial page
get their information? Luckily, Wikipedia articles for computer science
concepts often have links to the original papers that described or
introduced them. Follow those links\!

Also, when you read a paper, look closely at the papers they cite. A
well-written pair of Background and Related Works sections will be a
crash course in the state of the art of the topic at the time the paper
was written. Follow those links\!

## Source Quality

  - Best: peer-reviewed academic sources
    
    The top publishers of computer science research are
    [USENIX](https://www.usenix.org/), [the Association for Computing
    Machinery (ACM)](https://dl.acm.org/), and [the Institute of
    Electrical and Electronics Engineers
    (IEEE)](https://ieeexplore.ieee.org/Xplore/home.jsp). They have vast
    libraries of computer science papers that you can access from the
    campus networks.

  - Good: books

  - Good: well-known newspapers/magazines, especially if they have
    well-maintained online archives

  - Beware of link rot. Look for DOIs
    
    Beware of URLs that may change or disappear over time, such as the
    homepage of a company or software project.
    
    Most modern peer-reviewed papers will have a [*Digital Object
    Identifier
    (DOI)*](https://en.wikipedia.org/wiki/Digital_object_identifier)
    that is a more permanent link to that document in a reliable
    archive.

  - Useful as side sources: interviews or blogs from key people

  - Sloppy: Wikipedia, tutorial pages

The ideals to look for in a source are:

  - [x] Peer reviewed
  - [x] Unchanging
  - [x] Reliably archived

# Advice on Writing in General

The short paper “The ABC of academic writing: non-native speakers’
perspective”
([doi:10.1016/j.tree.2024.01.008](https://doi.org/10.1016/j.tree.2024.01.008))
offers some concise writing advice that is especially geared towards
those who are not native English speakers. This paper comes from the
field of ecology, but the general advice for writing should also apply
well to computer science.

Note that the paper recommends bouncing ideas off an AI tool like
ChatGPT. Remember that if you use ChatGPT or other LLM/AI tool, you must
disclose that and explain how you used it.

# Typesetting with LaTeX

We recommend that you begin using LaTeX to typeset your reports. LaTeX
is a programming language for documents, and its tools for typesetting
mathematical formulas are unparalleled. Because of this, it is the
de-facto standard for typesetting scientific papers in the fields of
computer science, mathematics, and physics.

The core TeX typesetting engine and macro language were developed by
legendary computer scientist and mathematician Donald Knuth. LaTeX is
actually a comprehensive set of TeX macros, developed by legendary
distributed-systems Leslie Lamport, that make it easier to separate
content from style, much like CSS does for the web. These days it is
relatively rare to use plain TeX without LaTeX. Plain TeX is probably
best left to the hard-core
[TeXnicians](https://tex.stackexchange.com/a/474039/229926).

TeX and LaTeX come from an earlier age of computer science, and their
syntax and semantics can seem arcane at times. But due to their
prevalence in academia, they is very much worth learning. LaTeX also
allows you to treat your documents like code: writing with your favorite
text editor and tracking changes with Git. As Mike likes to say: “LaTeX
is a great way to program about writing while you’re procrastinating on
writing about programming.”

TeX/LaTeX is available on most operating systems, typically in
distribution called [TeXLive](https://www.tug.org/texlive/). On
Ubuntu/Debian, for example, one just has to `apt install texlive` to
install most of the software you need.

There is a LaTeX report template included in the precode of each
assignment. You are welcome to use it to help you get started writing
your report.

From there, it is a matter of learning how to use LaTeX. There is a
wealth of information online. Good places to start include:

  - [The LaTeX Wikibook](https://en.wikibooks.org/wiki/LaTeX)
    
    An excellent, comprehensive, and free crowdsourced guide to LaTeX

  - [Overleaf’s LaTeX documentation](https://www.overleaf.com/learn)
    
    Overleaf is a popular web-based LaTeX editor and collaboration tool.
    It is a commercial service that offers free or paid accounts on its
    servers, but it is also possible to download the [open-source
    core](https://github.com/overleaf/overleaf) of their platform
    software and host your own Overleaf instance. It is in Overleaf’s
    interest to get more people using LaTeX, so they have put a lot of
    resources into thorough LaTeX documentation and tutorials, including
    nice quick-start guide [Learn LaTeX in 30
    Minutes](https://www.overleaf.com/learn/latex/Learn_LaTeX_in_30_minutes)

  - [TeX/LaTeX StackExchange](https://tex.stackexchange.com/)
    
    The StackOverflow/StackExchange family of Q\&A sites includes one
    dedicated to TeX and LaTeX.

  - [texfaq.org](https://texfaq.org/)
    
    A repository of frequently-asked questions and answers

  - [texdoc.org](https://texdoc.org/)
    
    A repository of TeX package documentation

  - [TeX User’s Group (TUG)](https://tug.org/)
    
    The non-profit group that is the steward of TeX and LaTeX’s
    development. They have many links to tools and documentation, and
    their journal [TUGboat](https://tug.org/TUGboat/) is the hub of TeX
    research and development.
