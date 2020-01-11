#!/bin/sh

# For pull requests, TRAVIS_BRANCH is the *target* branch, usually "master".
# This leads to mis-assigned coverage reports: they aren't assigned to PR's
# source branch. To combat this, we detect if the build is triggered by a PR,
# and explicitly use PR's source branch if so. That branch might not actually
# exist in our main repo, but that's fine -- Coveralls doesn't check.
#
# Similar distinction exists between TRAVIS_COMMIT and TRAVIS_PULL_REQUEST_SHA,
# but we don't paper over it there because we *do* want the report to attach to
# the commit that was actually built: this way, we (theoretically) can notice
# if the merge commit affected our coverage somehow.
if [ "${TRAVIS_EVENT_TYPE}" = "pull_request" ]
then
    branch="${TRAVIS_PULL_REQUEST_BRANCH}"
else
    branch="${TRAVIS_BRANCH}"
fi

if [ "${TRAVIS_PULL_REQUEST}" = "false" ]
then
    service_pull_request=""
else
    service_pull_request="--service-pull-request ${TRAVIS_PULL_REQUEST}"
fi

${HOME}/.cargo/bin/grcov \
    . \
    --service-name "travis-ci" \
    --service-number "${TRAVIS_BUILD_ID}" \
    --service-job-number "${TRAVIS_JOB_ID}" \
    --token "${COVERALLS_REPO_TOKEN}" \
    --commit-sha "${TRAVIS_COMMIT}" \
    --vcs-branch "${branch}" \
    ${service_pull_request} \
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
