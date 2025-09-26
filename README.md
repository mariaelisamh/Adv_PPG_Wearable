Hey guys!

For anyone new to the github repository system I reccomend you look up a tutorial to get yourself acquainted

Other than that there are a few things you should know:

You can update documents either directly in github or by importing the repository to github. 

Be careful to only update documents you know to be the only one working on currently or else you may overwrite someone else's work.

If you decide to import the repository do know that you will have to perioidically refresh with the upstream to ensure your code is up to date

Steps for importing repo:
download VS code 
and git bash : https://git-scm.com/downloads/win (x64 setup)
sign in with relevant info in vs code and in the github website
open the git bash terminal in vs code and type the following:

git config --global user.email "you@example.com" (email associated with your github acc)

git config --global user.name "Your Name" (username associated with your github account)

Now navigate to the repo and select <> code -> HTTPS -> copy this address

In git bash terminal in VS code type the following: git clone link
(replace link w the link)

