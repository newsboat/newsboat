#!/bin/sh

# For pull requests, TRAVIS_BRANCH is the *target* branch, usually "master".
# This leads to mis-assigned coverage reports. To combat this, we detect if the
# build is triggered by a PR, and explicitly use PR's source branch if so. We
# add PR's slug ("owner_name/repo_name") in front of the branch name, to avoid
# PRs from "master" branch to affect our master's coverage.
#
# Similar distinction exists between TRAVIS_COMMIT and TRAVIS_PULL_REQUEST_SHA,
# but we don't paper over it there because we *do* want the report to attach to
# the commit that was actually built: this way, we (theoretically) can notice
# if the merge commit affected our coverage somehow.
if [ "${TRAVIS_EVENT_TYPE}" = "pull_request" ]
then
    branch="${TRAVIS_PULL_REQUEST_SLUG}/${TRAVIS_PULL_REQUEST_BRANCH}"
else
    branch="${TRAVIS_BRANCH}"
fi

if [ "${TRAVIS_PULL_REQUEST}" = "true" ]
then
    service_pull_request="--service-pull-request ${TRAVIS_PULL_REQUEST}"
fi

if [ -n "${COVERALLS_REPO_TOKEN}" ]
then
    token="--token ${COVERALLS_REPO_TOKEN}"
fi

${HOME}/.cargo/bin/grcov \
    . \
    ${token} \
    ${service_pull_request} \
    --service-number "${TRAVIS_BUILD_ID}" \
    --service-name travis-ci \
    --service-job-number "${TRAVIS_JOB_ID}" \
    --commit-sha "${TRAVIS_COMMIT}" \
    --vcs-branch "${branch}" \
    --ignore-not-existing \
    --ignore='/*' \
    --ignore='3rd-party/*' \
    --ignore='doc/*' \
    --ignore='test/*' \
    --ignore='newsboat.cpp' \
    --ignore='podboat.cpp' \
    -t coveralls \
    -o coveralls.json

curl \
    --form "json_file=@coveralls.json" \
    --include \
    https://coveralls.io/api/v1/jobs
