## Contributing

Bug fixes, APIs changes, documentation, examples and test suite enhancements are some of the areas that can be contributed to this repository.

Details on the contribution process will be added shortly.

## License details

Please use an [SPDX license identifier](http://spdx.org/licenses/) in every source file following the [recommendations](https://spdx.dev/spdx-specification-21-web-version/#h.twlc0ztnng3b) to make it easier for users to understand and review licenses. 

The full original [Apache 2.0 license text](http://www.apache.org/licenses/LICENSE-2.0) is in [LICENSE-apache-2.0.txt](LICENSE-apache-2.0.txt). Each source header should start with your copyright line, the SPDX identifier and the Apache 2.0 header as shown here:

```
Copyright (c) [First year]-[Last year], **Your Name or Company Here**
SPDX-License-Identifier: Apache-2.0

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
either express or implied.

See the License for the specific language governing permissions and limitations under the License.
```

## Developer’s Certificate of Origin

Each commit must have at least one `Signed-off-by:` line from the committer to certify that the contribution is made under the terms of the [Developer Certificate of Origin](./DCO.txt).

If you set your `user.name` and `user.email` as part of your git configuration, you can add a signoff to your commit automatically with `git commit -s`. You have to use your real name (i.e., pseudonyms or anonymous contributions cannot be made). This is because the DCO is a legally binding document.

## Pull request guidelines

- Git commit messages should be concise and must contain a proper description of the commit content. We recommend following Chris Beams 7 rules https://chris.beams.io/posts/git-commit/#seven-rules.
- Add a prefix to your commit message to highlight the area of relevance. Write prefixes capitalized (only the first letter in uppercase, the rest in lowercase), followed by a colon, followed by a space, and then finally the commit title.
- Keep a linear git history: no merge commits are allowed, always rebase your branch.
- Preserve the history until the review is completed. Use `git commit --fixup` to add fixes. Once review is completed, run `git rebase --autosquash` to clean the history.
- Every new addition to an already opened pull request should be commented to inform the reviewers on what has changed.
